#include "utils.h"

memsvc_handle_t memsvc;

struct localArgs getLocalArgs(RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path       = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi  = lmdb::dbi::open(rtxn, nullptr);
    struct localArgs la = {.rtxn     = rtxn,
                           .dbi      = dbi,
                           .response = response};
    return la;
}

std::vector<std::string> split(const std::string &s, char delim)
{
  std::vector<std::string> elems;
  split(s, delim, std::back_inserter(elems));
  return elems;
}

std::string serialize(xhal::utils::Node n)
{
  return std::to_string((uint32_t)n.real_address)+"|"+n.permission+"|"+std::to_string((uint32_t)n.mask);
}

void update_address_table(const RPCMsg *request, RPCMsg *response) {
  LOGGER->log_message(LogManager::INFO, "START UPDATE ADDRESS TABLE");
  std::string at_xml = request->get_string("at_xml");
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb/data.mdb";
  std::string lmdb_lock_file = gem_path+"/address_table.mdb/lock.mdb";
  std::string lmdb_area_file = gem_path+"/address_table.mdb";
  xhal::utils::XHALXMLParser * m_parser = new xhal::utils::XHALXMLParser(at_xml.c_str());
  try {
    m_parser->setLogLevel(0);
    m_parser->parseXML();
  } catch (...) {
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
  env.open(lmdb_area_file.c_str(), 0, 0664);

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
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
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
  if (found) {
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

uint32_t getNumNonzeroBits(uint32_t value){
    //See: https://stackoverflow.com/questions/4244274/how-do-i-count-the-number-of-zero-bits-in-an-integer
    uint32_t numNonzeroBits=0;
    for(size_t i=0; i < CHAR_BIT * sizeof value; ++i){
        if ((value & (1 << i)) == 1){
            numNonzeroBits++;
        }
    }

    return numNonzeroBits;
} //End numNonzeroBits()

uint32_t getMask(localArgs * la, const std::string & regName){
    lmdb::val key, db_res;
    bool found=false;
    key.assign(regName.c_str());
    found = la->dbi.get(la->rtxn,key,db_res);
    uint32_t mask = 0x0;
    if (found){
        std::vector<std::string> tmp;
        std::string t_db_res = std::string(db_res.data());
        t_db_res = t_db_res.substr(0,db_res.size());
        tmp = split(t_db_res,'|');
        mask = stoll(tmp[2]);
    } else {
        LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
        la->response->set_string("error", "Register not found");
    }
    return mask;
} //End getMask(...)

void writeRawAddress(uint32_t address, uint32_t value, RPCMsg *response){
  uint32_t data[1];
  data[0] = value;
  if (memhub_write(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::INFO, stdsprintf("write memsvc error: %s", memsvc_get_last_error(memsvc)));
  }
}

uint32_t readRawAddress(uint32_t address, RPCMsg* response){
  uint32_t data[1];
  if (memhub_read(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
    return 0xdeaddead;
  }
  return data[0];
}

uint32_t getAddress(localArgs * la, const std::string & regName){
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = la->dbi.get(la->rtxn,key,db_res);
  uint32_t address;
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    address = stoi(tmp[0]);
  } else {
    LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    la->response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
  return address;
}

void writeAddress(lmdb::val & db_res, uint32_t value, RPCMsg *response) {
  std::vector<std::string> tmp;
  std::string t_db_res = std::string(db_res.data());
  t_db_res = t_db_res.substr(0,db_res.size());
  tmp = split(t_db_res,'|');
  uint32_t data[1];
  uint32_t address = stoi(tmp[0]);
  data[0] = value;
  if (memhub_write(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::INFO, stdsprintf("write memsvc error: %s", memsvc_get_last_error(memsvc)));
  }
}

uint32_t readAddress(lmdb::val & db_res, RPCMsg *response) {
  std::vector<std::string> tmp;
  std::string t_db_res = std::string(db_res.data());
  t_db_res = t_db_res.substr(0,db_res.size());
  tmp = split(t_db_res,'|');
  uint32_t data[1];
  uint32_t address = stoi(tmp[0]);
  int n_current_tries = 0;
  while (true)
  {
      if (memhub_read(memsvc, address, 1, data) != 0)
      {
          if (n_current_tries < 9)
          {
              n_current_tries++;
              LOGGER->log_message(LogManager::ERROR, stdsprintf("Reading reg %08X failed %i times.", address, n_current_tries));
          }
          else
          {
               response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
               LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s failed 10 times", memsvc_get_last_error(memsvc)));
               return 0xdeaddead;
          }
      }
      else break;
  }
  return data[0];
}

void writeRawReg(localArgs * la, const std::string & regName, uint32_t value) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = la->dbi.get(la->rtxn,key,db_res);
  if (found){
    writeAddress(db_res, value, la->response);
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    la->response->set_string("error", "Register not found");
  }
}

uint32_t readRawReg(localArgs * la, const std::string & regName) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = la->dbi.get(la->rtxn,key,db_res);
  if (found){
    return readAddress(db_res, la->response);
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    la->response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
}

uint32_t applyMask(uint32_t data, uint32_t mask) {
  uint32_t result = data & mask;
  for (int i = 0; i < 32; i++)
  {
    if (mask & 1)
    {
      break;
    }else {
      mask = mask >> 1;
      result = result >> 1;
    }
  }
  return result;
}

uint32_t readReg(localArgs * la, const std::string & regName) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = la->dbi.get(la->rtxn,key,db_res);
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    std::size_t found = tmp[1].find_first_of("r");
    if (found==std::string::npos) {
    	// response->set_string("error", std::string("No read permissions"));
    	LOGGER->log_message(LogManager::ERROR, stdsprintf("No read permissions for %s", regName.c_str()));
      return 0xdeaddead;
    }
    uint32_t data[1];
    uint32_t address,mask;
    address = stoll(tmp[0]);
    mask = stoll(tmp[2]);
    if (memhub_read(memsvc, address, 1, data) != 0) {
    	// response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
    	LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
      return 0xdeaddead;
    }
    if (mask!=0xFFFFFFFF) {
      return applyMask(data[0],mask);
    } else {
      return data[0];
    }
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    // response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
}

void writeReg(localArgs * la, const std::string & regName, uint32_t value) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = la->dbi.get(la->rtxn,key,db_res);
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    uint32_t mask = stoll(tmp[2]);
    if (mask==0xFFFFFFFF) {
      writeAddress(db_res, value, la->response);
    } else {
      uint32_t current_value = readAddress(db_res, la->response);
      if (current_value == 0xdeaddead) {
  	    la->response->set_string("error", std::string("Writing masked reg failed due to reading problem"));
  	    LOGGER->log_message(LogManager::ERROR, stdsprintf("Writing masked reg failed due to reading problem: %s", regName.c_str()));
        return;
      }
      int shift_amount = 0;
      uint32_t mask_copy = mask;
      for (int i = 0; i < 32; i++)
      {
        if (mask & 1)
        {
          break;
        } else {
          shift_amount +=1;
          mask = mask >> 1;
        }
      }
      uint32_t val_to_write = value << shift_amount;
      val_to_write = (val_to_write & mask_copy) | (current_value & ~mask_copy);
      writeAddress(db_res, val_to_write, la->response);
    }
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    la->response->set_string("error", "Register not found");
  }
}

extern "C" {
	const char *module_version_key = "utils v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memhub_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("utils", "update_address_table", update_address_table);
		modmgr->register_method("utils", "readRegFromDB", readRegFromDB);
	}
}
