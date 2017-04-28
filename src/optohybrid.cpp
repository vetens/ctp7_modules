#include "utils.h"
#include "vfat_parameters.h"
#include <unistd.h>

void broadcastWriteLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_write, uint32_t val_to_write, RPCMsg * response, uint32_t mask = 0xFF000000) {
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
  uint32_t mask = request->get_key_exists("mask")?request->get_word("mask"):0xFF000000;
  std::string oh_number = request->get_string("oh_number");
  broadcastWriteLocal(rtxn, dbi, oh_number, reg_to_write, val_to_write, response, mask);
  rtxn.abort();
}

// Set default values to VFAT parameters. VFATs will remain in sleep mode
void biasAllVFATsLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg *response, uint32_t mask = 0xFF000000) {
  for (auto & it:vfat_parameters)
  {
    broadcastWriteLocal(rtxn, dbi, oh_number, it.first, it.second, response, mask);
  }
}

void setAllVFATsToRunModeLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg * response, uint32_t mask = 0xFF000000) {
  broadcastWriteLocal(rtxn, dbi, oh_number, "ContReg0", 0x37, response, mask);
}

void setAllVFATsToSleepModeLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg * response, uint32_t mask = 0xFF000000) {
  broadcastWriteLocal(rtxn, dbi, oh_number, "ContReg0", 0x36, response, mask);
}

void loadVT1Local(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string config_file, RPCMsg * response, uint32_t vt1 = 0x64) {
  std::string reg_basename = "GEM_AMC.OH.OH" + oh_number;
  uint32_t vfatN, trimRange;
  std::string line, regName;
  // Check if there's a config file. If yes, set the thresholds and trim range according to it, otherwise st only thresholds (equal on all chips) to provided vt1 value
  if (config_file!="") {
    LOGGER->log_message(LogManager::INFO, stdsprintf("CONFIG FILE FOUND: %s", config_file.c_str()));
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
        //LOGGER->log_message(LogManager::INFO, stdsprintf("WRITING 0x%8x to REG: %s", vt1, regName.c_str()));
        writeRawReg(rtxn, dbi, regName, vt1, response);
        regName = reg_basename + ".GEB.VFATS.VFAT" + std::to_string(vfatN)+".ContReg3";
        //LOGGER->log_message(LogManager::INFO, stdsprintf("WRITING 0x%8x to REG: %s", trimRange, regName.c_str()));
        writeRawReg(rtxn, dbi, regName, trimRange, response);
      }
    }
  } else {
    LOGGER->log_message(LogManager::INFO, "CONFIG FILE NOT FOUND");
    broadcastWriteLocal(rtxn, dbi, oh_number, "VThreshold1", vt1, response);
  }
}

void loadVT1(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string oh_number = request->get_string("oh_number");
  std::string config_file = request->get_key_exists("thresh_config_filename")?request->get_string("thresh_config_filename"):"";
  uint32_t vt1 = request->get_key_exists("vt1")?request->get_word("vt1"):0x64;
  loadVT1Local(rtxn, dbi, oh_number, config_file, response, vt1);
  rtxn.abort();
}

void loadTRIMDACLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string config_file, RPCMsg * response) {
  std::string reg_basename = "GEM_AMC.OH.OH" + oh_number + ".GEB.VFATS.VFAT";
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
      regName = reg_basename + std::to_string(vfatN)+".VFATChannels.ChanReg" + std::to_string(vfatCH);
      writeRawReg(rtxn, dbi, regName, trim + 32*mask, response);
    }
  }
}

void loadTRIMDAC(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string oh_number = request->get_string("oh_number");
  std::string config_file = request->get_string("trim_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
  loadTRIMDACLocal(rtxn, dbi, oh_number, config_file, response);
  rtxn.abort();
}

void configureVFATs(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string oh_number = request->get_string("oh_number");
  std::string trim_config_file = request->get_string("trim_config_filename");//"/mnt/persistent/texas/test/chConfig_GEMINIm01L1.txt";
  std::string thresh_config_file = request->get_key_exists("thresh_config_filename")?request->get_string("thresh_config_filename"):"";
  uint32_t vt1 = request->get_key_exists("vt1")?request->get_word("vt1"):0x64;
  LOGGER->log_message(LogManager::INFO, "BIAS VFATS");
  biasAllVFATsLocal(rtxn, dbi, oh_number, response);
  LOGGER->log_message(LogManager::INFO, "LOAD VT1 VFATS");
  loadVT1Local(rtxn, dbi, oh_number, thresh_config_file, response, vt1);
  LOGGER->log_message(LogManager::INFO, "LOAD TRIM VFATS");
  loadTRIMDACLocal(rtxn, dbi, oh_number, trim_config_file, response);
  if (request->get_key_exists("set_run")) setAllVFATsToRunModeLocal(rtxn, dbi, oh_number, response);
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
		modmgr->register_method("optohybrid", "configureVFATs", configureVFATs);
	}
}
