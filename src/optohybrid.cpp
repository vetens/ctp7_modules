#include "utils.h"
#include "vfat_parameters.h"
#include <unistd.h>

void broadcastWriteLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_write, uint32_t mask, uint32_t val_to_write, RPCMsg * response) {
  std::string reg_basename = "GEM_AMC.OH.OH";
  reg_basename += oh_number;
  reg_basename += ".GEB.Broadcast";
  std::string regName;

  //Reset broadcast module
  regName = reg_basename + ".Reset";
  writeRawReg(rtxn, dbi, regName, 0, response);
  //Set broadcast mask
  regName = reg_basename + ".Mask";
  writeRawReg(rtxn, dbi, regName, mask, response);
  //Issue broadcast write request
  regName = reg_basename + ".Request." + reg_to_write;
  writeRawReg(rtxn, dbi, regName, val_to_write, response);
  //Wait until broadcast write finishes
  regName = reg_basename + ".Running";
  while (unsigned int t_res = readRawReg(rtxn, dbi, regName, response))
  {
    if (t_res == 0xdeaddead) break;
    usleep(1000);
  }
}

void broadcastWrite(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string reg_to_write = request->get_string("reg_name");
  uint32_t val_to_write = request->get_word("value");
  uint32_t mask = request->get_word("mask");
  std::string oh_number = request->get_string("oh_number");
  broadcastWriteLocal(rtxn, dbi, oh_number, reg_to_write, mask, val_to_write, response);
  rtxn.abort();
}

void loadVT1(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string reg_basename = "GEM_AMC.OH.OH";
  std::string oh_number = request->get_string("oh_number");
  reg_basename += oh_number;
  std::string config_file = "";
  uint32_t vfatN, vt1, trimRange;
  std::string line, regName;

  // Check if there's a config file. If yes, set the thresholds and trim range according to it, otherwise st only thresholds (equal on all chips) to provided vt1 value
  if (request->get_key_exists("vfat_config_filename")) {
    config_file = request->get_string("vfat_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
    std::ifstream infile(config_file);
    std::getline(infile,line);// skip first line
    while (std::getline(infile,line))
    {
      std::stringstream iss(line);
      if (!(iss >> vfatN >> vt1 >> trimRange)) {
  		  LOGGER->log_message(LogManager::ERROR, "ERROR READING SETTINGS");
        response->set_string("error", "Error reading settings");
        break;
      } else {
        regName = reg_basename + ".GEB.VFATS.VFAT" + std::to_string(vfatN)+".VThreshold1";
        writeRawReg(rtxn, dbi, regName, vt1, response);
        regName = reg_basename + ".GEB.VFATS.VFAT" + std::to_string(vfatN)+".ContReg3";
        writeRawReg(rtxn, dbi, regName, trimRange, response);
      }
    }
  } else {
    vt1 = request->get_word("vt1");
    broadcastWriteLocal(rtxn, dbi, oh_number, "VThreshold1", 0xff000000, vt1, response);
  }
  rtxn.abort();
}

void loadTRIMDAC(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string reg_basename = "GEM_AMC.OH.OH";
  std::string oh_number = request->get_string("oh_number");
  reg_basename += oh_number;
  std::string config_file = request->get_string("trim_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
  std::ifstream infile(config_file);
  std::string line, regName;
  uint32_t vfatN, vfatCH, trim, mask;
  std::getline(infile,line);// skip first line
  while (std::getline(infile,line))
  {
    std::stringstream iss(line);
    if (!(iss >> vfatN >> vfatCH >> trim >> mask)) {
		  LOGGER->log_message(LogManager::ERROR, "ERROR READING SETTINGS");
      response->set_string("error", "Error reading settings");
      break;
    } else {
      regName = reg_basename + ".GEB.VFATS.VFAT" + std::to_string(vfatN)+".VFATChannels.ChanReg" + std::to_string(vfatCH);
      writeRawReg(rtxn, dbi, regName, trim + 32*mask, response);
    }
  }
  rtxn.abort();
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
		modmgr->register_method("optohybrid", "broadcastWrite", broadcastWrite);
		modmgr->register_method("optohybrid", "loadVT1", loadVT1);
		modmgr->register_method("optohybrid", "loadTRIMDAC", loadTRIMDAC);
	}
}
