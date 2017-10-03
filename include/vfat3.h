#include "utils.h"
#include <string>

void configureVFATsLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, uint32_t oh_number, uint32_t vfat_pos, std::string config_file, RPCMsg * response) {
  std::string reg_basename = "GEM_AMC.OH.OH" + std::to_string(oh_number) + ".GEB.VFAT"+std::to_string(vfat_pos);
  std::ifstream infile(config_file);
  std::string line, regName;
  uint32_t dacVal;
  std::string dacName;
  std::getline(infile,line);// skip first line
  while (std::getline(infile,line))
  {
    std::stringstream iss(line);
    if (!(iss >> dacName >> dacVal)) {
		  LOGGER->log_message(LogManager::ERROR, "ERROR READING SETTINGS");
      response->set_string("error", "Error reading settings");
      break;
    } else {
      regName = reg_basename + dacName;
      writeReg(rtxn, dbi, regName, dacVal, response);
    }
  }
}

void configureVFATs(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  uint32_t oh_number = request->get_word("oh_number");
  uint32_t vfat_pos = request->get_word("vfat_pos");
  std::string config_file = request->get_key_exists("config_filename")?request->get_string("config_filename"):"/mnt/persistent/configuration/vfat3/default.txt";
  LOGGER->log_message(LogManager::INFO, "Load VFAT configuration");
  configureVFATsLocal(rtxn, dbi, oh_number, vfat_pos, config_file, response);
  // Eventually add the method below later
  //if (request->get_key_exists("set_run")) setAllVFATsToRunModeLocal(rtxn, dbi, oh_number, response);
  rtxn.abort();
}
