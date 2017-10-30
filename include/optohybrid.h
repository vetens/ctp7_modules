#include "utils.h"
#include "vfat_parameters.h"
#include <unistd.h>

void broadcastWriteLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_write, uint32_t val_to_write, RPCMsg * response, uint32_t mask = 0xFF000000) {
  uint32_t fw_maj = readReg(rtxn, dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  if (fw_maj == 2) {
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
  } else if (fw_maj == 3) {
    std::string reg_basename = "GEM_AMC.OH.OH";
    reg_basename += oh_number;
    reg_basename += ".GEB.VFAT";
    std::string regName;
    for (int i=0; i<24; i++){
      if (!((mask >> i)&0x1)) {
        regName = reg_basename + std::to_string(i)+"."+reg_to_write;
        writeReg(rtxn, dbi, regName, val_to_write, response);
      }
    }
  } else {
    LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
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
  std::string oh_number = std::to_string(request->get_word("oh_number"));
  broadcastWriteLocal(rtxn, dbi, oh_number, reg_to_write, val_to_write, response, mask);
  rtxn.abort();
}

void broadcastReadLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_read, RPCMsg * response, uint32_t mask = 0xFF000000) {
  uint32_t fw_maj = readReg(rtxn, dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
  std::string reg_basename = "GEM_AMC.OH.OH";
  reg_basename += oh_number;
  if (fw_maj == 2) {
    reg_basename += ".GEB.VFATS.VFAT";
  } else if (fw_maj == 3) {
    reg_basename += ".GEB.VFAT";
   } else {
    LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
    response->set_string("error", "Unexpected value for system release major!");
  }
  std::string regName;
  uint32_t data[24];
  for (int i=0; i<24; i++){
    if ((mask >> i)&0x1) data[i] = 0;
    else {
      regName = reg_basename + std::to_string(i)+"."+reg_to_read;
      data[i] = readReg(rtxn, dbi, regName);
      if (data[i] == 0xdeaddead) response->set_string("error",stdsprintf("Error reading register %s",regName.c_str()));
    }
  }
  response->set_word_array("data", data, 24);
}

void broadcastRead(const RPCMsg *request, RPCMsg *response) {
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  std::string reg_to_read = request->get_string("reg_name");
  uint32_t mask = request->get_key_exists("mask")?request->get_word("mask"):0xFF000000;
  std::string oh_number = std::to_string(request->get_word("oh_number"));
  broadcastReadLocal(rtxn, dbi, oh_number, reg_to_read, response, mask);
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

void configureScanModuleLocal(localArgs * la, uint32_t ohN, uint32_t vfatN, uint32_t scanmode, bool useUltra, uint32_t mask, uint32_t ch, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep){
    /*
     *     Configure the firmware scan controller
     *      mode: 0 Threshold scan
     *            1 Threshold scan per channel
     *            2 Latency scan
     *            3 s-curve scan
     *            4 Threshold scan with tracking data
     *      vfat: for single VFAT scan, specify the VFAT number
     *            for ULTRA scan, specify the VFAT mask
     */

    //Set Scan Base
    std::string scanBase = "GEM_AMC.OH.OH" + ohN + ".ScanController";
    (useUltra)?scanBase += ".ULTRA":scanBase += ".THLAT";

    // check if another scan is running
    if (readRawReg(la->rtxn, la->dbi, scanBase + ".MONITOR.STATUS", la->response) > 0) {
      LOGGER->log_message(LogManager::WARNING, stdsprintf("%s: Scan is already running, not starting a new scan", scanBase.c_str()));
      la->response->set_string("error", "Scan is already running, not starting a new scan");
      return;
    }
    // reset scan module
    writeRawReg(la->rtxn, la->dbi, scanBase + ".RESET", 0x1, la->response);

    // write scan parameters
    writeReg(la->rtxn, la->dbi, scanBase + ".MODE", scanmode, la->response);
    if (useUltra){
        writeReg(la->rtxn, la->dbi, scanBase + ".MASK", mask, la->response);
    else{
        writeReg(la->rtxn, la->dbi, scanBase + ".CHIP", vfatN, la->response);
    }
    writeReg(la->rtxn, la->dbi, scanBase + ".CHAN", ch, la->response);
    writeReg(la->rtxn, la->dbi, scanBase + ".NTRIGS", nevts, la->response);
    writeReg(la->rtxn, la->dbi, scanBase + ".MIN", dacMin, la->response);
    writeReg(la->rtxn, la->dbi, scanBase + ".MAX", dacMax, la->response);
    writeReg(la->rtxn, la->dbi, scanBase + ".STEP", dacStep, la->response);

    return;
} //End configureScanModuleLocal()

void configureScanModule(const RPCMsg *request, RPCMsg *response){
    /*
     *     Configure the firmware scan controller
     *      mode: 0 Threshold scan
     *            1 Threshold scan per channel
     *            2 Latency scan
     *            3 s-curve scan
     *            4 Threshold scan with tracking data
     *      vfat: for single VFAT scan, specify the VFAT number
     *            for ULTRA scan, specify the VFAT mask
     */

    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    //Get OH and scanmode
    std::string ohN = request->get_string("ohN");
    uint32_t scanmode = request->get_word("scanmode");

    //Setup ultra mode, mask, and/or vfat number
    bool useUltra = false;
    uint32_t mask = 0xFFFFFFFF;
    uint32_t vfatN = 0;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
        mask = request->get_word("mask");
    }
    else{
        vfatN = request->get_word("vfatN");
    }

    uint32_t channel = request->get_word("ch");
    uint32_t numtrigs = request->get_word("nevts");
    uint32_t scanmin = request->get_word("dacMin");
    uint32_t scanmax = request->get_word("dacMax");
    uint32_t stepsize = request->get_word("dacStep");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    configureScanModuleLocal(&la, ohN, vfatN, scanmode, useUltra, mask, ch, nevts, dacMin, dacMax, dacStep);

    return;
}

void startScanModule(lmdb::txn & rtxn, lmdb::dbi & dbi, RPCMsg *response)
{
}
