/*! \file amc.cpp
 *  \brief AMC methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#include "amc.h"
#include <chrono>
#include <map>
#include <string>
#include <time.h>
#include <thread>
#include "utils.h"
#include <vector>

std::vector<uint32_t> sbitReadOutLocal(localArgs *la, uint32_t ohN, uint32_t acquireTime, bool *maxNetworkSizeReached){
    //Setup the sbit monitor
    const int nclusters = 8;
    writeReg(la, "GEM_AMC.TRIGGER.SBIT_MONITOR.OH_SELECT", ohN);
    uint32_t addrSbitMonReset=getAddress(la, "GEM_AMC.TRIGGER.SBIT_MONITOR.RESET");
    uint32_t addrSbitL1ADelay=getAddress(la, "GEM_AMC.TRIGGER.SBIT_MONITOR.L1A_DELAY");
    uint32_t addrSbitCluster[nclusters];
    for(int iCluster=0; iCluster < nclusters; ++iCluster){
        addrSbitCluster[iCluster] = getAddress(la, stdsprintf("GEM_AMC.TRIGGER.SBIT_MONITOR.CLUSTER%i",iCluster) );
    }

    //Take the VFATs out of slow control only mode
    writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

    //[0:10] address of sbit cluster
    //[11:13] cluster size
    //[14:26] L1A Delay (consider anything over 4095 as overflow)
    std::vector<uint32_t> storedSbits;

    //readout sbits
    time_t acquisitionTime,startTime;
    bool acquire=true;
    startTime=time(NULL);
    (*maxNetworkSizeReached) = false;
    uint32_t l1ADelay;
    while(acquire){
        if( sizeof(uint32_t) * storedSbits.size() > 65000 ){ //Max TCP/IP message is 65535
            (*maxNetworkSizeReached) = true;
            break;
        }

        //Reset monitors
        writeRawAddress(addrSbitMonReset, 0x1, la->response);

        //wait for 4095 clock cycles then read L1A delay
        std::this_thread::sleep_for(std::chrono::nanoseconds(4095*25));
        l1ADelay = readRawAddress(addrSbitL1ADelay, la->response);
        if(l1ADelay > 4095){ //Anything larger than this consider as overflow
            l1ADelay = 4095; //(0xFFF in hex)
        }

        //get sbits
        bool anyValid=false;
        std::vector<uint32_t> tempSBits; //will only be stored into storedSbits if anyValid is true
        for(int cluster=0; cluster<nclusters; ++cluster){
            //bits [10:0] is the address of the cluster
            //bits [14:12] is the cluster size
            //bits 15 and 11 are not used
            uint32_t thisCluster = readRawAddress(addrSbitCluster[cluster], la->response);
            uint32_t sbitAddress = (thisCluster & 0x7ff);
            int clusterSize = (thisCluster >> 12) & 0x7;
            bool isValid = (sbitAddress < 1536); //Possible values are [0,(24*64)-1]

            if(isValid){
                LOGGER->log_message(LogManager::INFO,stdsprintf("valid sbit data: thisClstr %x; sbitAddr %x;",thisCluster,sbitAddress));
                anyValid=true;
            }

            //Store the sbit
            tempSBits.push_back( (l1ADelay << 14) + (clusterSize << 12) + sbitAddress);
        } //End Loop over clusters

        if(anyValid){
            storedSbits.insert(storedSbits.end(),tempSBits.begin(),tempSBits.end());
        }

        acquisitionTime=difftime(time(NULL),startTime);
        if(acquisitionTime > acquireTime){
            acquire=false;
        }
    } //End readout sbits

    return storedSbits;
} //End sbitReadOutLocal(...)

void sbitReadOut(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t acquireTime = request->get_word("acquireTime");

    bool maxNetworkSizeReached = false;
    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    time_t startTime=time(NULL);
    std::vector<uint32_t> storedSbits = sbitReadOutLocal(&la, ohN, acquireTime, &maxNetworkSizeReached);
    time_t approxLivetime=difftime(time(NULL),startTime);

    if(maxNetworkSizeReached){
        response->set_word("maxNetworkSizeReached", maxNetworkSizeReached);
        response->set_word("approxLiveTime",approxLivetime);
    }
    response->set_word_array("storedSbits",storedSbits);

    return;
} //End sbitReadOut()

unsigned int fw_version_check(const char* caller_name, localArgs *la)
{
    int iFWVersion = readReg(la, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
    char regBuf[200];
    switch (iFWVersion){
        case 1:
        {
            LOGGER->log_message(LogManager::INFO, "System release major is 1, v2B electronics behavior");
            break;
        }
        case 3:
        {
            LOGGER->log_message(LogManager::INFO, "System release major is 3, v3 electronics behavior");
            break;
        }
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
            sprintf(regBuf,"Unexpected value for system release major!");
            la->response->set_string("error",regBuf);
            break;
        }
    }
    return iFWVersion;
}

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

void getmonTRIGGERmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  la->response->set_word("OR_TRIGGER_RATE",readReg(la,"GEM_AMC.TRIGGER.STATUS.OR_TRIGGER_RATE"));
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.TRIGGER_RATE",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.TRIGGER_RATE",std::to_string(i).c_str());
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
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGERmainLocal(&la, NOH);
  rtxn.abort();
}

void getmonTRIGGEROHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.LINK0_MISSED_COMMA_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_MISSED_COMMA_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_MISSED_COMMA_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_MISSED_COMMA_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_OVERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_OVERFLOW_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_OVERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_OVERFLOW_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_UNDERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_UNDERFLOW_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_UNDERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_UNDERFLOW_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK0_SBIT_OVERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK0_SBIT_OVERFLOW_CNT",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.LINK1_SBIT_OVERFLOW_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.TRIGGER.OH%s.LINK1_SBIT_OVERFLOW_CNT",std::to_string(i).c_str());
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
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonTRIGGEROHmainLocal(&la, NOH);
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

void getmonDAQOHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.STATUS.EVT_SIZE_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.EVENT_FIFO_HAD_OFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_OFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.INPUT_FIFO_HAD_UFLOW",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_TOO_MANY",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.%s",t1.c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.STATUS.VFAT_NO_MARKER",std::to_string(i).c_str());
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
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonDAQOHmainLocal(&la, NOH);
  rtxn.abort();
}

void getmonOHmainLocal(localArgs * la, int NOH)
{
  std::string t1,t2;
  for (int i = 0; i < NOH; i++){
    t1 = stdsprintf("OH%s.FW_VERSION",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.STATUS.FW.VERSION",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_COUNTER",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVN",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.EVENT_RATE",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.EVT_RATE",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRK_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRK_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GTX.TRG_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GTX_LINK.TRG_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.GBT.TRK_ERR",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.OH.OH%s.COUNTERS.GBT_LINK.TRK_ERR",std::to_string(i).c_str());
    la->response->set_word(t1,readReg(la,t2));
    t1 = stdsprintf("OH%s.CORR_VFAT_BLK_CNT",std::to_string(i).c_str());
    t2 = stdsprintf("GEM_AMC.DAQ.OH%s.COUNTERS.CORRUPT_VFAT_BLK_CNT",std::to_string(i).c_str());
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
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHmainLocal(&la, NOH);
  rtxn.abort();
}

void getmonOHSCAmainLocal(localArgs *la, uint32_t NOH){
    std::string strRegName, strKeyName;

    for (unsigned int ohN = 0; ohN < NOH; ++ohN){ //Loop over all optohybrids
        //Log Message
        LOGGER->log_message(LogManager::INFO, stdsprintf("Reading SCA Monitoring Values for OH%i",ohN));

        //SCA Temperature
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.SCA_TEMP",ohN);
        strKeyName = stdsprintf("OH%i.SCA_TEMP",ohN);
        la->response->set_word(strKeyName,readReg(la, strRegName));

        //OH Temperature Sensors
        for(int tempVal=0; tempVal <= 9; ++tempVal){ //Loop over optohybrid temperatures sensosrs
            strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.BOARD_TEMP%i",ohN,tempVal);
            strKeyName = stdsprintf("OH%i.BOARD_TEMP%i",ohN,tempVal);
            la->response->set_word(strKeyName, readReg(la, strRegName));
        } //End Loop over optohybrid temeprature sensors

        //Voltage Monitor - AVCCN
        strRegName = stdsprintf("GEM_AMC.SLOW_CONTROL.SCA.ADC_MONITORING.OH%i.AVCCN",ohN);
        strKeyName = stdsprintf("OH%i.AVCCN",ohN);
        la->response->set_word(strKeyName, readReg(la, strRegName));

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
  struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
  getmonOHSCAmainLocal(&la, NOH);
  rtxn.abort();
}

uint32_t getOHVFATMaskLocal(localArgs * la, uint32_t ohN){
    uint32_t mask = 0x0;
    for(int vfatN=0; vfatN<24; ++vfatN){ //Loop over all vfats
        uint32_t syncErrCnt = readReg(la, stdsprintf("GEM_AMC.OH_LINKS.OH%i.VFAT%i.SYNC_ERR_CNT",ohN,vfatN));

        if(syncErrCnt > 0x0){ //Case: nonzero sync errors, mask this vfat
            mask = mask + (0x1 << vfatN);
        } //End Case: nonzero sync errors, mask this vfat
    } //End loop over all vfats

    return mask;
} //End getOHVFATMaskLocal()

void getOHVFATMask(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    LOGGER->log_message(LogManager::INFO, stdsprintf("Determining VFAT Mask for OH%i",ohN));
    uint32_t vfatMask = getOHVFATMaskLocal(&la, ohN);

    response->set_word("vfatMask",vfatMask);

    return;
} //End getOHVFATMask(...)

void getOHVFATMaskMultiLink(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohMask = request->get_word("ohMask");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    uint32_t ohVfatMaskArray[12];
    for(int ohN=0; ohN<12; ++ohN){
        // If this Optohybrid is masked skip it
        if(((ohMask >> ohN) & 0x0)){
            continue;
        }

        ohVfatMaskArray[ohN] = getOHVFATMaskLocal(&la, ohN);
    } //End Loop over all Optohybrids

    response->set_word_array("ohVfatMaskArray",ohVfatMaskArray,12);

    return;
} //End getOHVFATMaskMultiLink(...)

extern "C" {
    const char *module_version_key = "amc v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memsvc_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("amc", "getmonTTCmain", getmonTTCmain);
        modmgr->register_method("amc", "getmonTRIGGERmain", getmonTRIGGERmain);
        modmgr->register_method("amc", "getmonTRIGGEROHmain", getmonTRIGGEROHmain);
        modmgr->register_method("amc", "getmonDAQmain", getmonDAQmain);
        modmgr->register_method("amc", "getmonDAQOHmain", getmonDAQOHmain);
        modmgr->register_method("amc", "getmonOHmain", getmonOHmain);
        modmgr->register_method("amc", "getOHVFATMask", getOHVFATMask);
        modmgr->register_method("amc", "getOHVFATMaskMultiLink", getOHVFATMaskMultiLink);
        modmgr->register_method("amc", "sbitReadOut", sbitReadOut);
    }
}
