#include "moduleapi.h"
#include <libmemsvc.h>
#include "lmdb_cpp_wrapper.h"
#include "xhal/utils/XHALXMLParser.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string serialize(xhal::utils::Node n)
{
  return std::to_string((uint32_t)n.real_address)+"|"+n.permission+"|"+std::to_string((uint32_t)n.mask);
}

void update_address_table(const RPCMsg *request, RPCMsg *response) {
  std::string at_xml = request->get_string("at_xml");
  xhal::utils::XHALXMLParser * m_parser = new xhal::utils::XHALXMLParser(at_xml.c_str());
  try 
  {
    m_parser->setLogLevel(0);
    m_parser->parseXML();
  } catch (...){
    response->set_string("error", "XML parser failed");
    LOGGER->log_message(LogManager::INFO, "XML parser failed");
    return;
  }
  std::unordered_map<std::string,xhal::utils::Node> m_parsed_at;
  m_parsed_at = m_parser->getAllNodes();
  m_parsed_at.erase("top");
  xhal::utils::Node t_node;

  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);

  lmdb::val key;
  lmdb::val value;
  auto wtxn = lmdb::txn::begin(env);
  auto dbi = lmdb::dbi::open(wtxn, nullptr);
  dbi.drop(wtxn);
  wtxn.commit();

  std::string t_key;
  std::string t_value;
  for (auto it:m_parsed_at)
  {
    t_key = it.first;
    t_key = t_key.substr(4);
    t_node = it.second;
    t_value = serialize(t_node);
    key.assign(t_key);
    value.assign(t_value);
    dbi.put(wtxn, key, value);
  }
  wtxn.commit();
}

extern "C" {
	const char *module_version_key = "utils v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("utils", "update_address_table", update_address_table);
	}
}
