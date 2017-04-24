#include "moduleapi.h"
#include <libmemsvc.h>
#include <fstream>

memsvc_handle_t memsvc;

void loadTRIMDAC(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/example.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  lmdb::val value_;
  bool found;
  std::string reg_basename = "GEM_AMC.OH.OH";
  std::string oh_number = request->get_string("oh_number");
  reg_basename += oh_number;
  std::string config_file = "/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
  std::ifstream infile(config_file);
  std::string line;
  int vfatN, vfatCH, trim, mask;
  std::getline(infile,line);// skip first line
  while (std::getline(infile,line))
  {
    std::stringstream iss(line);
    if (!(iss >> vfatN >> vfatCH >> trim >> mask)) {
      std::cout << "ERROR SETTINGS READING" << std::endl;
      break;
    } else {
      std::cout << "VFAT Number " << vfatN << " Channel " << vfatCH << " Trim Value " << trim << " Mask Value " << mask << std::endl;
    }
  }
}

void mblockread(const RPCMsg *request, RPCMsg *response) {
  uint32_t count = request->get_word("count");
  uint32_t addr = request->get_word("address");
  uint32_t data[count];

  for (unsigned int i=0; i<count; i++){
    if (memsvc_read(memsvc, addr, 1, &data[i]) != 0) {
      response->set_string("error", memsvc_get_last_error(memsvc));
      LOGGER->log_message(LogManager::INFO, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
      return;
    }
  }
	response->set_word_array("data", data, count);
}
void mlistread(const RPCMsg *request, RPCMsg *response) {
  uint32_t count = request->get_word("count");
  uint32_t addr[count];
  request->get_word_array("addresses", addr);
  uint32_t data[count];

  for (unsigned int i=0; i<count; i++){
    if (memsvc_read(memsvc, addr[i], 1, &data[i]) != 0) {
      response->set_string("error", memsvc_get_last_error(memsvc));
      LOGGER->log_message(LogManager::INFO, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
      return;
    }
  }
	response->set_word_array("data", data, count);
}
extern "C" {
	const char *module_version_key = "optohybrid v1.0.1";
	int module_activity_color = 4;
	void module_init(ModuleManager *modmgr) {
		if (memsvc_open(&memsvc) != 0) {
			LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
			LOGGER->log_message(LogManager::ERROR, "Unable to load module");
			return; // Do not register our functions, we depend on memsvc.
		}
		modmgr->register_method("optohybrid", "blockread", mblockread);
		//modmgr->register_method("optohybrid", "listread", mlistread);
	}
}
