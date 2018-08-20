/*! \file amc.cpp
 *  \brief AMC methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 */

#include <chrono>
#include <map>
#include <time.h>
#include <thread>
#include "utils.h"
#include <vector>

/*! \fn std::vector<uint32_t> sbitReadOutLocal(localArgs *la, uint32_t ohN, uint32_t acquireTime, bool *maxNetworkSizeReached)
 *  \brief reads out sbits from optohybrid ohN for a number of seconds given by acquireTime and returns them to the user.
 *  \details The SBIT Monitor stores the 8 SBITs that are sent from the OH (they are all sent at the same time and correspond to the same clock cycle). Each SBIT clusters readout from the SBIT Monitor is a 16 bit word with bits [0:10] being the sbit address and bits [12:14] being the sbit size, bits 11 and 15 are not used.
 *  \details The possible values of the SBIT Address are [0,1535].  Clusters with address less than 1536 are considered valid (e.g. there was an sbit); otherwise an invalid (no sbit) cluster is returned.  The SBIT address maps to a given trigger pad following the equation \f$sbit = addr % 64\f$.  There are 64 such trigger pads per VFAT.  Each trigger pad corresponds to two VFAT channels.  The SBIT to channel mapping follows \f$sbit=floor(chan/2)\f$.  You can determine the VFAT position of the sbit via the equation \f$vfatPos=7-int(addr/192)+int((addr%192)/64)*8\f$.
 *  \details The SBIT size represents the number of adjacent trigger pads are part of this cluster.  The SBIT address always reports the lowest trigger pad number in the cluster.  The sbit size takes values [0,7].  So an sbit cluster with address 13 and with size of 2 includes 3 trigger pads for a total of 6 vfat channels and starts at channel \f$13*2=26\f$ and continues to channel \f$(2*15)+1=31\f$.
 *  \details The output vector will always be of size N * 8 where N is the number of readouts of the SBIT Monitor.  For each readout the SBIT Monitor will be reset and then readout after 4095 clock cycles (~102.4 microseconds).  The SBIT clusters stored in the SBIT Monitor will not be over-written until a module reset is sent.  The readout will stop before acquireTime finishes if the size of the returned vector approaches the max TCP/IP size (~65000 btyes) and sets maxNetworkSize to true.
 *  \details Each element of the output vector will be a 32 bit word.  Bits [0,10] will the address of the SBIT Cluster, bits [11:13] will be the cluster size, and bits [14:26] will be the difference between the SBIT and the input L1A (if any) in clock cycles.  While the SBIT Monitor stores the difference between the SBIT and input L1A as a 32 bit number (0xFFFFFFFF) any value higher 0xFFF (12 bits) will be truncated to 0xFFF.  This matches the time between readouts of 4095 clock cycles.
 *  \param la Local arguments structure
 *  \param ohN Optical link
 *  \param acquireTime acquisition time on the wall clock in seconds
 *  \param maxNetworkSize pointer to a boolean, set to true if the returned vector reaches a byte count of 65000
 */
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
            }

            //Store the sbit
            storedSbits.push_back( (l1ADelay << 14) + (clusterSize << 12) + sbitAddress);
        } //End Loop over clusters

        acquisitionTime=difftime(time(NULL),startTime);
        if(acquisitionTime > acquireTime){
            acquire=false;
        }
    } //End readout sbits

    return storedSbits;
} //End sbitReadOutLocal(...)

/*! \fn sbitReadOut(const RPCMsg *request, RPCMsg *response)
 *  \brief readout sbits using the SBIT Monitor.  See the local callable methods documentation for details.
 *  \param request RPC response message
 *  \param response RPC response message
 */
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

/*! \fn void getmonTTCmainLocal(localArgs * la)
 *  \brief Local version of getmonTTCmain
 *  \param la Local arguments
 */
void getmonTTCmainLocal(localArgs * la)
{
  LOGGER->log_message(LogManager::INFO, "Called getmonTTCmainLocal");
  la->response->set_word("MMCM_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.MMCM_LOCKED"));
  la->response->set_word("TTC_SINGLE_ERROR_CNT",readReg(la,"GEM_AMC.TTC.STATUS.TTC_SINGLE_ERROR_CNT"));
  la->response->set_word("BC0_LOCKED",readReg(la,"GEM_AMC.TTC.STATUS.BC0.LOCKED"));
  la->response->set_word("L1A_ID",readReg(la,"GEM_AMC.TTC.L1A_ID"));
  la->response->set_word("L1A_RATE",readReg(la,"GEM_AMC.TTC.L1A_RATE"));
}

/*! \fn void getmonTTCmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of TTC monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
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

/*! \fn void getmonTRIGGERmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonTRIGGERmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
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

/*! \fn void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
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

/*! \fn void getmonTRIGGEROHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonTRIGGEROHmain
 *  * LINK{0,1}_SBIT_OVERFLOW_CNT -- this is an interesting counter to monitor from operations perspective, but is not related to the health of the link itself. Rather it shows how many times OH had too many clusters which it couldn't fit into the 8 cluster per BX bandwidth. If this counter is going up it just means that OH is seeing a very high hit occupancy, which could be due to high radiation background, or thresholds configured too low.
 *
 *  The other 3 counters reflect the health of the optical links:
 *  * LINK{0,1}_OVERFLOW_CNT and LINK{0,1}_UNDERFLOW_CNT going up indicate a clocking issue, specifically they go up when the frequency of the clock on the OH doesn't match the frequency on the CTP7. Given that CTP7 is providing the clock to OH, this in principle should not happen unless the OH is sending complete garbage and thus the clock cannot be recovered on CTP7 side, or the bit-error rate is insanely high, or the fiber is just disconnected, or OH is off. In other words, this indicates a critical problem.
 *  * LINK{0,1}_MISSED_COMMA_CNT also monitors the health of the link, but it's more sensitive, because it can go up due to isolated single bit errors. With radiation around, this might count one or two counts in a day or two. But if it starts running away, that would indicate a real problem, but in this case most likely the overflow and underflow counters would also see it.
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
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

/*! \fn void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
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

/*! \fn void getmonDAQmainLocal(localArgs * la)
 *  \brief Local version of getmonDAQmain
 *  \param la Local arguments
 */
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

/*! \fn void getmonDAQmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
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

/*! \fn void getmonDAQOHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonDAQOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
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

/*! \fn void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
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

/*! \fn void getmonOHmainLocal(localArgs * la, int NOH)
 *  \brief Local version of getmonOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 */
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

/*! \fn void getmonOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of OH monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
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
	}
}
