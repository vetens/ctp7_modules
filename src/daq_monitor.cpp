/*! \file amc.cpp
 *  \brief AMC methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#include "daq_monitor.h"
#include <string>
#include "utils.h"

void getmonTTCmainLocal(localArgs * la)
{
  LOGGER->log_message(LogManager::INFO, "Called getmonTTCmainLocal");
  la->response->set_word("MMCM_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.MMCM_LOCKED"));
  la->response->set_word("TTC_SINGLE_ERROR_CNT",readReg(la,"GEM_AMC.TTC.STATUS.TTC_SINGLE_ERROR_CNT"));
  la->response->set_word("BC0_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.BC0.LOCKED"));
  la->response->set_word("L1A_ID",readReg(la,"GEM_AMC.TTC.L1A_ID"));
  la->response->set_word("L1A_RATE",readReg(la,"GEM_AMC.TTC.L1A_RATE"));
}

void getmonTTCmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTTCmainLocal(&la);
  rtxn.abort();
}

void getmonTRIGGERmainLocal(localArgs * la, int NOH, int ohMask)
{
  std::string t1,t2;
  la->response->set_word("OR_TRIGGER_RATE",readReg(la,"GEM_AMC.TRIGGER.STATUS.OR_TRIGGER_RATE"));
  for (int ohN = 0; ohN < NOH; ohN++){
    // If this Optohybrid is masked skip it
    if(!((ohMask >> ohN) & 0x1)){
      continue;
    }
    t1 = stdsprintf("OH%s.TRIGGER_RATE",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.TRIGGER_RATE",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGERmainLocal(&la, NOH, ohMask);
  rtxn.abort();
}

void getmonTRIGGEROHmainLocal(localArgs * la, int NOH, int ohMask)
{
  std::string t1,t2;
  for (int ohN = 0; ohN < NOH; ohN++){
    // If this Optohybrid is masked skip it
    if(!((ohMask >> ohN) & 0x1)){
      continue;
    }
    t1 = stdsprintf("OH%s.LINK0_MISSED_COMMA_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_MISSED_COMMA_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_MISSED_COMMA_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_MISSED_COMMA_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_OVERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_OVERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_OVERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_OVERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_UNDERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_UNDERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_UNDERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_UNDERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_SBIT_OVERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_SBIT_OVERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_SBIT_OVERFLOW_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_SBIT_OVERFLOW_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGEROHmainLocal(&la, NOH, ohMask);
  rtxn.abort();
}

void getmonDAQmainLocal(localArgs * la)
{
  la->response->set_word("DAQ_ENABLE",readReg(la,"GEM_AMC.DAQ.CONTROL.DAQ_ENABLE"));
  la->response->set_word("DAQ_LINK_READY",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_LINK_RDY"));
  la->response->set_word("DAQ_LINK_AFULL",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_LINK_AFULL"));
  la->response->set_word("DAQ_OFIFO_HAD_OFLOW",readReg(la,"GEM_AMC.DAQ.STATUS.DAQ_OUTPUT_FIFO_HAD_OVERFLOW"));
  la->response->set_word("L1A_FIFO_HAD_OFLOW",readReg(la,"GEM_AMC.DAQ.STATUS.L1A_FIFO_HAD_OVERFLOW"));
  la->response->set_word("L1A_FIFO_DATA_COUNT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.L1A_FIFO_DATA_CNT"));
  la->response->set_word("DAQ_FIFO_DATA_COUNT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.DAQ_FIFO_DATA_CNT"));
  la->response->set_word("EVENT_SENT",readReg(la,"GEM_AMC.DAQ.EXT_STATUS.EVT_SENT"));
  la->response->set_word("TTS_STATE",readReg(la,"GEM_AMC.DAQ.STATUS.TTS_STATE"));
}

void getmonDAQmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonDAQmainLocal(&la);
  rtxn.abort();
}

void getmonDAQOHmainLocal(localArgs * la, int NOH, int ohMask)
{
  std::string t1,t2;
  for (int ohN = 0; ohN < NOH; ohN++){
    // If this Optohybrid is masked skip it
    if(!((ohMask >> ohN) & 0x1)){
      continue;
    }
    t1 = stdsprintf("OH%s.STATUS.EVT_SIZE_ERR",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.EVENT_FIFO_HAD_OFLOW",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_OFLOW",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_UFLOW",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_TOO_MANY",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_NO_MARKER",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonDAQOHmainLocal(&la, NOH, ohMask);
  rtxn.abort();
}

void getmonOHmainLocal(localArgs * la, int NOH, int ohMask)
{
  std::string t1,t2;
  for (int ohN = 0; ohN < NOH; ohN++){
    // If this Optohybrid is masked skip it
    if(!((ohMask >> ohN) & 0x1)){
      continue;
    }
    t1 = stdsprintf("OH%s.FW_VERSION",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.STATUS.FW.VERSION",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_COUNTER",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVN",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_RATE",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVT_RATE",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRK_ERR",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRK_ERR",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRG_ERR",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRG_ERR",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GBT.TRK_ERR",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GBT_LINK.TRK_ERR",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.CORR_VFAT_BLK_CNT",std::to_string(ohN).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.CORRUPT_VFAT_BLK_CNT",std::to_string(ohN).c_str());
    la->response->set_word(t1,readReg(la,t2));
  }
}

void getmonOHmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHmainLocal(&la, NOH, ohMask);
  rtxn.abort();
}

void getmonOHSCAmainLocal(localArgs *la, int NOH, int ohMask){
    std::string strRegName, strKeyName;

    for (int ohN = 0; ohN < NOH; ++ohN){ //Loop over all optohybrids
        // If this Optohybrid is masked skip it
        if(!((ohMask >> ohN) & 0x1)){
            continue;
        }

        //Log Message
        LOGGER->log_message(LogManager::INFO, stdsprintf("Reading SCA Monitoring Values for OH%i",ohN));

        //SCA Temperature
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.SCA_TEMP",ohN);
        strKeyName = stdsprintf("OH%i.SCA_TEMP",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegName));

        //OH Temperature Sensors
        for(int tempVal=1; tempVal <= 9; ++tempVal){ //Loop over optohybrid temperatures sensosrs
            strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.BOARD_TEMP%i",ohN,tempVal);
            strKeyName = stdsprintf("OH%i.BOARD_TEMP%i",ohN,tempVal);
            la->response->set_word(strKeyName, readReg(la, strRegName));
        } //End Loop over optohybrid temeprature sensors

        //Voltage Monitor - AVCCN
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.AVCCN",ohN);
        strKeyName = stdsprintf("OH%i.AVCCN",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - AVTTN
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.AVTTN",ohN);
        strKeyName = stdsprintf("OH%i.AVTTN",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 1V0_INT
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.1V0_INT",ohN);
        strKeyName = stdsprintf("OH%i.1V0_INT",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 1V8F
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.1V8F",ohN);
        strKeyName = stdsprintf("OH%i.1V8F",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 1V5
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.1V5",ohN);
        strKeyName = stdsprintf("OH%i.1V5",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 2V5_IO
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.2V5_IO",ohN);
        strKeyName = stdsprintf("OH%i.2V5_IO",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 3V0
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.3V0",ohN);
        strKeyName = stdsprintf("OH%i.3V0",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - 1V8
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.1V8",ohN);
        strKeyName = stdsprintf("OH%i.1V8",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - VTRX_RSSI2
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.VTRX_RSSI2",ohN);
        strKeyName = stdsprintf("OH%i.VTRX_RSSI2",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

        //Voltage Monitor - VTRX_RSSI1
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.VTRX_RSSI1",ohN);
        strKeyName = stdsprintf("OH%i.VTRX_RSSI1",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));
    } //End Loop over all optohybrids

    return;
} //End getmonOHSCAmainLocal(...)

void getmonOHSCAmain(const RPCMsg *request, RPCMsg *response)
{
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHSCAmainLocal(&la, NOH, ohMask);
  rtxn.abort();
}

void getmonOHSysmonLocal(localArgs *la, int NOH, int ohMask, bool doReset){
    std::string strKeyName;
    std::string strRegBase;

    for (int ohN = 0; ohN < NOH; ++ohN){ //Loop over all optohybrids
        // If this Optohybrid is masked skip it
        if(!((ohMask >> ohN) & 0x1)){
            continue;
        }

        //Set regBase
        strRegBase = stdsprintf("GEM_AMC.OH.OH%i.FPGA.ADC.CTRL.",ohN);

        //Log Message
        LOGGER->log_message(LogManager::INFO, stdsprintf("Reading Sysmon Values for OH%i",ohN));

        //Issue reset??
        if(doReset){
            writeReg(la, strRegBase+"RESET", 0x1);
        }

        //Read Alarm conditions & counters - OVERTEMP
        strKeyName = stdsprintf("OH%i.OVERTEMP",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "OVERTEMP"));

        strKeyName = stdsprintf("OH%i.CNT_OVERTEMP",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "CNT_OVERTEMP"));

        //Read Alarm conditions & counters - VCCAUX_ALARM
        strKeyName = stdsprintf("OH%i.VCCAUX_ALARM",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "VCCAUX_ALARM"));

        strKeyName = stdsprintf("OH%i.CNT_VCCAUX_ALARM",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "CNT_VCCAUX_ALARM"));

        //Read Alarm conditions & counters - VCCINT_ALARM
        strKeyName = stdsprintf("OH%i.VCCINT_ALARM",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "VCCINT_ALARM"));

        strKeyName = stdsprintf("OH%i.CNT_VCCINT_ALARM",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegBase + "CNT_VCCINT_ALARM"));

        //Enable Sysmon ADC Read
        writeReg(la, strRegBase + "ENABLE", 0x1);

        //Read Sysmon Values - Core Temperature
        writeReg(la, strRegBase + "ADR_IN", 0x0);
        strKeyName = stdsprintf("OH%i.FPGA_CORE_TEMP");
        la->response->set_word(strKeyName,readReg(la, strRegBase + "DATA_OUT"));

        //Read Sysmon Values - Core Voltage
        writeReg(la, strRegBase + "ADR_IN", 0x1);
        strKeyName = stdsprintf("OH%i.FPGA_CORE_1V0");
        la->response->set_word(strKeyName,readReg(la, strRegBase + "DATA_OUT"));

        //Read Sysmon Values - I/O Voltage
        writeReg(la, strRegBase + "ADR_IN", 0x2);
        strKeyName = stdsprintf("OH%i.FPGA_CORE_2V5_IO");
        la->response->set_word(strKeyName,readReg(la, strRegBase + "DATA_OUT"));

        //Disable Sysmon ADC Read
        writeReg(la, strRegBase + "ENABLE", 0x0);
    } //End Loop over all optohybrids
} //End getmonOHSysmonLocal()

void getmonOHSysmon(const RPCMsg *request, RPCMsg *response){
  auto env = lmdb::env::create();
  env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
  std::string gem_path = std::getenv("GEM_PATH");
  std::string lmdb_data_file = gem_path+"/address_table.mdb";
  env.open(lmdb_data_file.c_str(), 0, 0664);
  auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
  auto dbi = lmdb::dbi::open(rtxn, nullptr);
  int NOH = request->get_word("NOH");

  int ohMask = 0xfff;
  if(request->get_key_exists("ohMask")){
    ohMask = request->get_word("ohMask");
  }

  bool doReset = request->get_word("doReset");

  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHSysmonLocal(&la, NOH, ohMask, doReset);
  rtxn.abort();
} //End getmonOHSysmon()

extern "C" {
    const char *module_version_key = "daq_monitor v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memsvc_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("daq_monitor", "getmonTTCmain", getmonTTCmain);
        modmgr->register_method("daq_monitor", "getmonTRIGGERmain", getmonTRIGGERmain);
        modmgr->register_method("daq_monitor", "getmonTRIGGEROHmain", getmonTRIGGEROHmain);
        modmgr->register_method("daq_monitor", "getmonDAQmain", getmonDAQmain);
        modmgr->register_method("daq_monitor", "getmonDAQOHmain", getmonDAQOHmain);
        modmgr->register_method("daq_monitor", "getmonOHmain", getmonOHmain);
        modmgr->register_method("daq_monitor", "getmonOHSCAmain", getmonOHSCAmain);
        modmgr->register_method("daq_monitor", "getmonOHSysmon", getmonOHSysmon);
    }
}
