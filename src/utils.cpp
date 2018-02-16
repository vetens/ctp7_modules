#include "utils.h"

void update_address_table(const RPCMsg *request, RPCMsg *response) {
  LOGGER->log_message(LogManager::INFO, "START UPDATE ADDRESS TABLE");
  std::string at_xml = request->get_string("at_xml");
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  std::string lmdb_lock_file = gem_path+"address_table.mdb/lock.mdb";
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
  LOGGER->log_message(LogManager::INFO, "XML PARSING DONE");
  std::unordered_map<std::string,xhal::utils::Node> m_parsed_at;
  m_parsed_at = m_parser->getAllNodes();
  m_parsed_at.erase("top");
  xhal::utils::Node t_node;

  // Remove old DB
  LOGGER->log_message(LogManager::INFO, "REMOVE OLD DB");
  std::remove(lmdb_data_file.c_str());
  std::remove(lmdb_lock_file.c_str());

  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open(lmdb_data_file.c_str(), 0, 0664);

  LOGGER->log_message(LogManager::INFO, "LMDB ENV OPEN");

  lmdb::val key;
  lmdb::val value;
  auto wtxn = lmdb::txn::begin(env);
  auto dbi = lmdb::dbi::open(wtxn, nullptr);

  LOGGER->log_message(LogManager::INFO, "START ITERATING OVER MAP");

  std::string t_key;
  std::string t_value;
  for (auto it:m_parsed_at)
  {
    t_key = it.first;
    t_node = it.second;
    t_value = serialize(t_node);
    key.assign(t_key);
    value.assign(t_value);
    dbi.put(wtxn, key, value);
  }
  wtxn.commit();
  LOGGER->log_message(LogManager::INFO, "COMMIT DB");
  wtxn.abort();
}

void readRegFromDB(const RPCMsg *request, RPCMsg *response) {
  std::string regName = request->get_string("reg_name");
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  LOGGER->log_message(LogManager::INFO, "LMDB ENV OPEN");
  lmdb::val key;
  lmdb::val value;
  auto rtxn = lmdb::txn::begin(env);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  key.assign(regName.c_str());
  bool found = dbi.get(rtxn,key,value);
  uint32_t reg_address, reg_mask;
  std::string permissions;
  std::vector<std::string> temp;
  std::string t_value;
  if (found){
		LOGGER->log_message(LogManager::INFO, stdsprintf("Key: %s is found", regName.c_str()));
    t_value = std::string(value.data());
    t_value = t_value.substr(0,value.size());
    temp = split(t_value,'|');
    reg_address = stoll(temp[0]);
    permissions = temp[1];
    reg_mask = stoll(temp[2]);
    response->set_string("permissions", permissions);
    response->set_word("address", reg_address);
    response->set_word("mask", reg_mask);
  } else {
		LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    response->set_string("error", "Register not found");
  }
  rtxn.abort();
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
		modmgr->register_method("utils", "readRegFromDB", readRegFromDB);
	}
}
