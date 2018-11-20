/*! \file amc.cpp
 *  \brief AMC methods for RPC modules
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#include "amc.h"
#include <chrono>
#include <string>
#include <time.h>
#include <thread>
#include <utility>
#include "utils.h"
#include <vector>

uint32_t checkPLLLockLocal(localArgs * la, uint32_t readAttempts){
    uint32_t lockCnt = 0;
    for (uint32_t i = 0; i < readAttempts; ++i ) {
        writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_PLL_RESET", 0x1);

        //wait 100us to allow the PLL to lock
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        //Check if it's locked
        if (readReg(la,"GEM_AMC.TTC.STATUS.CLK.PHASE_LOCKED") != 0){
            lockCnt += 1;
        }
    }
    return lockCnt;
} //End checkPLLLockLocal()

void checkPLLLock(const RPCMsg * request, RPCMsg * response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t readAttempts = request->get_word("readAttempts");
    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    uint32_t lockCnt = checkPLLLockLocal(&la, readAttempts);
    LOGGER->log_message(LogManager::INFO, stdsprintf("Checked PLL Locked Status %i times; Found PLL Locked %i times",readAttempts,lockCnt));

    response->set_word("lockCnt",lockCnt);

    return;
} //End checkPLLLock()

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
    uint32_t vfatMask = getOHVFATMaskLocal(&la, ohN);
    LOGGER->log_message(LogManager::INFO, stdsprintf("Determined VFAT Mask for OH%i to be 0x%x",ohN,vfatMask));

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

    int ohMask = 0xfff;
    if(request->get_key_exists("ohMask")){
        ohMask = request->get_word("ohMask");
    }

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    unsigned int NOH = readReg(&la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (request->get_key_exists("NOH")){
        unsigned int NOH_requested = request->get_word("NOH");
        if (NOH_requested <= NOH)
            NOH = NOH_requested;
        else
            LOGGER->log_message(LogManager::WARNING, stdsprintf("NOH requested (%i) > NUM_OF_OH AMC register value (%i), NOH request will be disregarded",NOH_requested,NOH));
    }

    uint32_t ohVfatMaskArray[12];
    for(unsigned int ohN=0; ohN<NOH; ++ohN){
        // If this Optohybrid is masked skip it
        if(!((ohMask >> ohN) & 0x1)){
            ohVfatMaskArray[ohN] = 0xffffff;
            continue;
        }
        else{
            ohVfatMaskArray[ohN] = getOHVFATMaskLocal(&la, ohN);
            LOGGER->log_message(LogManager::INFO, stdsprintf("Determined VFAT Mask for OH%i to be 0x%x",ohN,ohVfatMaskArray[ohN]));
        }
    } //End Loop over all Optohybrids

    //Debugging
    LOGGER->log_message(LogManager::DEBUG, "All VFAT Masks found, listing:");
    for(int ohN=0; ohN<12; ++ohN){
        LOGGER->log_message(LogManager::DEBUG, stdsprintf("VFAT Mask for OH%i to be 0x%x",ohN,ohVfatMaskArray[ohN]));
    }

    response->set_word_array("ohVfatMaskArray",ohVfatMaskArray,12);

    return;
} //End getOHVFATMaskMultiLink(...)

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
            tempSBits.push_back( ((l1ADelay & 0x1fff) << 14) + ((clusterSize & 0x7) << 11) + (sbitAddress & 0x7ff) );
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

void ttcMMCMPhaseShiftLocal(localArgs *la, bool shiftOutOfLockFirst, bool useBC0Locked, bool doScan){
  const int PLL_LOCK_READ_ATTEMPTS  = 10;

  LOGGER->log_message(LogManager::INFO,stdsprintf("starting phase shifting procedure, paramters are:"
                                                  " shiftOutOfLockFirst %i, useBC0Locked %i, doScan %i",
                                                  shiftOutOfLockFirst,useBC0Locked,doScan)
                      );
  if (shiftOutOfLockFirst)
    LOGGER->log_message(LogManager::INFO,stdsprintf("will force a relock by shifting out of lock first"));
  if (useBC0Locked)
    LOGGER->log_message(LogManager::INFO,stdsprintf("will use the BC0 locked method"));
  if (doScan)
    LOGGER->log_message(LogManager::INFO,stdsprintf("will perform a phase scan"));


  std::string strTTCCtrlBaseNode = "GEM_AMC.TTC.CTRL.";
  std::vector<std::pair<std::string, uint32_t> > vec_ttcCtrlRegs;

  vec_ttcCtrlRegs.push_back(std::make_pair("DISABLE_PHASE_ALIGNMENT"       , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_DISABLE_GTH_PHASE_TRACKING" , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_MANUAL_OVERRIDE"            , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_MANUAL_SHIFT_DIR"           , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_GTH_MANUAL_OVERRIDE"        , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_GTH_MANUAL_SHIFT_DIR"       , 0x0));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_GTH_MANUAL_SHIFT_STEP"      , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_GTH_MANUAL_SEL_OVERRIDE"    , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_GTH_MANUAL_COMBINED"        , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("GTH_TXDLYBYPASS"               , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("PA_MANUAL_PLL_RESET"           , 0x1));
  vec_ttcCtrlRegs.push_back(std::make_pair("CNT_RESET"                     , 0x1));

  // write & readback of aforementioned registers
  uint32_t readback;
  for (auto &ttcRegIter: vec_ttcCtrlRegs) {
    writeReg(la, strTTCCtrlBaseNode + ttcRegIter.first, ttcRegIter.second);
    if ((ttcRegIter.first).rfind("RESET") != std::string::npos)
      continue;
    readback = readReg(la, strTTCCtrlBaseNode + ttcRegIter.first);
    if ( readback != ttcRegIter.second) {
      std::stringstream errmsg;
      errmsg << "Readback of "       << (strTTCCtrlBaseNode + ttcRegIter.first).c_str()
             << " failed: value is " << readback
             << ", expected "        << ttcRegIter.second;
      LOGGER->log_message(LogManager::ERROR, errmsg.str());
      la->response->set_string("error","ttcMMCMPhaseShift: " + errmsg.str());
      return;
    }
  }

  if (readReg(la,strTTCCtrlBaseNode+"DISABLE_PHASE_ALIGNMENT") == 0x0) {
    std::stringstream errmsg;
    errmsg << "automatic phase alignment is turned off!!";
    LOGGER->log_message(LogManager::ERROR, errmsg.str());
    la->response->set_string("error","ttcMMCMPhaseShift: " + errmsg.str());
    return;
  }

  uint32_t readAttempts = 1;
  int      maxShift     = 7680+(7680/2);

  if (!useBC0Locked) {
    readAttempts = PLL_LOCK_READ_ATTEMPTS;
  }
  if (doScan) {
    readAttempts = PLL_LOCK_READ_ATTEMPTS;
    maxShift = 23040;
  }

  uint32_t mmcmShiftCnt     = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_SHIFT_CNT");
  uint32_t gthShiftCnt      = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_GTH_SHIFT_CNT");
  int      pllLockCnt       = checkPLLLockLocal(la, readAttempts);
  bool     firstUnlockFound = false;
  bool     nextLockFound    = false;
  bool     bestLockFound    = false;
  bool     reversingForLock = false;
  uint32_t phase            = 0;
  double   phaseNs          = 0.0;

  bool mmcmShiftTable[] = {false, false, false, true, false, false, false,
                           false, false, true, false, false, false, false,
                           false, true, false, false, false, false, true,
                           false, false, false, false, false, true, false,
                           false, false, false, false, true, false, false,
                           false, false, false, true, false, false};

  int nGoodLocks       = 0;
  int nShiftsSinceLock = 0;
  int nBadLocks        = 0;
  int totalShiftCount  = 0;

  for (int i = 0; i < maxShift; ++i) {
    writeReg(la, strTTCCtrlBaseNode + "PA_GTH_MANUAL_SHIFT_EN", 0x1);
    uint32_t tmpGthShiftCnt  = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_GTH_SHIFT_CNT");
    uint32_t tmpMmcmShiftCnt = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_SHIFT_CNT");

    bool mmcmShiftRequired = false;
    if (reversingForLock)
      mmcmShiftRequired = mmcmShiftTable[gthShiftCnt];
    else
      mmcmShiftRequired = mmcmShiftTable[gthShiftCnt+1];

    std::stringstream msg;
    msg << "BEFORE: "
        << "mmcmShift: "  << mmcmShiftCnt << "(" << tmpMmcmShiftCnt << ")"
        << ", gthShift: " << gthShiftCnt  << "(" << tmpGthShiftCnt  << ")"
        << ", mmcmShiftRequired: "               << mmcmShiftRequired;
    LOGGER->log_message(LogManager::DEBUG,msg.str());

    if (!reversingForLock && (gthShiftCnt == 39)) {
      LOGGER->log_message(LogManager::DEBUG,"normal GTH shift rollover 39->0");
      gthShiftCnt = 0;
    } else if (reversingForLock && (gthShiftCnt == 0)){
      LOGGER->log_message(LogManager::DEBUG,"rerversed GTH shift rollover 0->39");
      gthShiftCnt = 39;
    } else {
      if (reversingForLock) {
        gthShiftCnt -= 1;
      } else {
        gthShiftCnt += 1;
      }
    }

    LOGGER->log_message(LogManager::DEBUG,stdsprintf("tmpGthShiftCnt: %i, tmpMmcmShiftCnt %i",
                                                     tmpGthShiftCnt, tmpMmcmShiftCnt));
    while (gthShiftCnt != tmpGthShiftCnt) {
      LOGGER->log_message(LogManager::NOTICE,
                          "Repeating a GTH PI shift because the shift count doesn't"
                          " match the expected value."
                          " Expected shift cnt = " + std::to_string(gthShiftCnt) +
                          ", ctp7 returned "       + std::to_string(tmpGthShiftCnt));

      writeReg(la, "GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_EN", 0x1);
      tmpGthShiftCnt = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_GTH_SHIFT_CNT");
      //FIX ME should this continue indefinitely...?
    }

    if (mmcmShiftRequired) {
      if (!reversingForLock && (mmcmShiftCnt == 0xffff)) {
        mmcmShiftCnt = 0;
      } else if (reversingForLock && (mmcmShiftCnt == 0x0)) {
        mmcmShiftCnt = 0xffff;
      } else {
        if (reversingForLock) {
          mmcmShiftCnt -= 1;
        } else {
          mmcmShiftCnt += 1;
        }
      }
    }

    tmpMmcmShiftCnt = readReg(la,"GEM_AMC.TTC.STATUS.CLK.PA_MANUAL_SHIFT_CNT");

    msg.str("");
    msg.clear();
    msg << "AFTER: "
        << "mmcmShift: "  << mmcmShiftCnt << "(" << tmpMmcmShiftCnt << ")"
        << ", gthShift: " << gthShiftCnt  << "(" << tmpGthShiftCnt  << ")"
        << ", mmcmShiftRequired: "               << mmcmShiftRequired;
    LOGGER->log_message(LogManager::DEBUG,msg.str().c_str());

    if (mmcmShiftCnt != tmpMmcmShiftCnt){
      LOGGER->log_message(LogManager::NOTICE,
                          "Reported MMCM shift count doesn't match the expected MMCM shift count."
                          " Expected shift cnt = " + std::to_string(mmcmShiftCnt) +
                          " , ctp7 returned "      + std::to_string(tmpMmcmShiftCnt));
    }

    pllLockCnt = checkPLLLockLocal(la,readAttempts);
    phase      = readReg(la,"GEM_AMC.TTC.STATUS.CLK.TTC_PM_PHASE_MEAN");
    phaseNs    = phase * 0.01860119;
    uint32_t gthPhase = readReg(la,"GEM_AMC.TTC.STATUS.CLK.GTH_PM_PHASE_MEAN");
    double gthPhaseNs = gthPhase * 0.01860119;

    uint32_t bc0Locked  = readReg(la,"GEM_AMC.TTC.STATUS.BC0.LOCKED");
    //uint32_t bc0UnlkCnt = readReg(la,"GEM_AMC.TTC.STATUS.BC0.UNLOCK_CNT");
    //uint32_t sglErrCnt  = readReg(la,"GEM_AMC.TTC.STATUS.TTC_SINGLE_ERROR_CNT");
    //uint32_t dblErrCnt  = readReg(la,"GEM_AMC.TTC.STATUS.TTC_DOUBLE_ERROR_CNT");

    LOGGER->log_message(LogManager::DEBUG,
                        "GTH shift #"             + std::to_string(i) +
                        ": mmcm shift cnt = "     + std::to_string(mmcmShiftCnt) +
                        ", mmcm phase counts = "  + std::to_string(phase) +
                        ", mmcm phase = "         + std::to_string(phaseNs) +
                        "ns, gth phase counts = " + std::to_string(gthPhase) +
                        ", gth phase = "          + std::to_string(gthPhaseNs) +
                        ", PLL lock count = "     + std::to_string(pllLockCnt)+
                        " bad locks "             + std::to_string(nBadLocks) +
                        ", good locks "           + std::to_string(nGoodLocks) +
                        " nextLockFound "         + std::to_string(nextLockFound) +
                        ", shiftsSinceLock "      + std::to_string(nShiftsSinceLock) +
                        ", totalShiftCount "      + std::to_string(totalShiftCount)
                        );

    if (useBC0Locked) {
      if (!firstUnlockFound) {
        bestLockFound = false;
        if (bc0Locked == 0) {
          nBadLocks += 1;
          nGoodLocks = 0;
        } else {
          nBadLocks   = 0;
          nGoodLocks += 1;
        }

        if (shiftOutOfLockFirst) {
          if (nBadLocks > 100) {
            firstUnlockFound = true;
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] 100 unlocks found after " + std::to_string(i+1) + " shifts:"
                                " bad locks "              + std::to_string(nBadLocks) +
                                ", good locks "            + std::to_string(nGoodLocks) +
                                ", mmcm phase count = "    + std::to_string(phase) +
                                ", mmcm phase ns = "       + std::to_string(phaseNs) + "ns");
          }
        } else {
          if (reversingForLock && (nBadLocks > 0)) {
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] Bad BC0 lock found:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns"
                                ", returning to normal search");
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",1);
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",0);
            bestLockFound    = false;
            reversingForLock = false;
            nGoodLocks       = 0;
          } else if (nGoodLocks == 200) {
            reversingForLock = true;
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] 200 consecutive good BC0 locks found:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns"
                                ", reversing scan direction");
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",0);
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",1);
          }
          if (reversingForLock && (nGoodLocks == 300)) {
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] Best lock found after reversing:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns.");
            bestLockFound    = true;
            if (doScan) {
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",1);
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",0);
              bestLockFound    = false;
              reversingForLock = false;
              nGoodLocks       = 0;
            } else {
              break;
            }
          }
        }
      } else { // shift to first good BC0 locked
        if (bc0Locked == 0) {
          if (nextLockFound) {
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] Unexpected unlock after " + std::to_string(i+1) + " shifts:"
                                " bad locks "              + std::to_string(nBadLocks) +
                                ", good locks "            + std::to_string(nGoodLocks) +
                                ", BC0 locked "            + std::to_string(bc0Locked) +
                                ", mmcm phase count = "    + std::to_string(phase) +
                                ", mmcm phase ns = "       + std::to_string(phaseNs) + "ns");
          }
          nBadLocks += 1;
        } else {
          if (!nextLockFound) {
            LOGGER->log_message(LogManager::INFO,
                                "[BC0Locked method] Found next lock after " + std::to_string(i+1) + " shifts:"
                                " bad locks "            + std::to_string(nBadLocks) +
                                ", good locks "          + std::to_string(nGoodLocks) +
                                ", BC0 locked "          + std::to_string(bc0Locked) +
                                ", mmcm phase count = "  + std::to_string(phase) +
                                ", mmcm phase ns = "     + std::to_string(phaseNs) + "ns");
            nextLockFound = true;
            nBadLocks   = 0;
          }
          nGoodLocks += 1;
        }
        if (nGoodLocks == 1920) {
          LOGGER->log_message(LogManager::INFO,
                              "[BC0Locked method] Finished 1920 shifts after first good lock: "
                              "bad locks "   + std::to_string(nBadLocks) +
                              " good locks " + std::to_string(nGoodLocks));
          bestLockFound = true;
          if (doScan) {
            nextLockFound    = false;
            firstUnlockFound = false;
            nGoodLocks       = 0;
            nBadLocks        = 0;
            nShiftsSinceLock = 0;
          } else {
            break;
          }
        }
      }
    } else if (true) { // using the PLL lock counter, but the method as for the BC0 lock
      if (!firstUnlockFound) {
        bestLockFound = false;
        if (pllLockCnt < PLL_LOCK_READ_ATTEMPTS) {
          nBadLocks += 1;
          nGoodLocks = 0;
        } else {
          nBadLocks   = 0;
          nGoodLocks += 1;
        }
        if (shiftOutOfLockFirst) {
          if (nBadLocks > 500) {
            firstUnlockFound = true;
            LOGGER->log_message(LogManager::INFO,
                                "[Using PLL lock method] 500 unlocks found after " + std::to_string(i+1) + " shifts:"
                                " bad locks "              + std::to_string(nBadLocks) +
                                ", good locks "            + std::to_string(nGoodLocks) +
                                ", mmcm phase count = "    + std::to_string(phase) +
                                ", mmcm phase ns = "       + std::to_string(phaseNs) + "ns");
          }
        } else {
          if (reversingForLock && (nBadLocks > 0)) {
            LOGGER->log_message(LogManager::INFO,
                                "[Using PLL lock method] Bad PLL lock found:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns"
                                ", returning to normal search");
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",1);
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",0);
            bestLockFound    = false;
            reversingForLock = false;
            nGoodLocks       = 0;
          } else if (nGoodLocks == 50) {
            reversingForLock = true;
            LOGGER->log_message(LogManager::INFO,
                                "[Using PLL lock method] 50 consecutive good PLL locks found:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns"
                                ", reversing scan direction");
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",0);
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",1);
          }
          if (reversingForLock &&(nGoodLocks == 75)) {
            LOGGER->log_message(LogManager::INFO,
                                "[Using PLL lock method] Best lock found after reversing:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns.");
            bestLockFound = true;
            if (doScan) {
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",1);
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",0);
              bestLockFound    = false;
              reversingForLock = false;
              nGoodLocks       = 0;
            } else {
              break;
            }  // end if doScan block
          }  // end if reversingForLock && (nGoodLocks == 75)
        }  // end if shiftOutOfLockFirst
      } else { // shift to first good PLL locked
        if (pllLockCnt < PLL_LOCK_READ_ATTEMPTS) {
          if (nextLockFound) {
            LOGGER->log_message(LogManager::NOTICE,
                                "[Using PLL lock method] Unexpected unlock after " + std::to_string(i+1) + " shifts:"
                                " bad locks "              + std::to_string(nBadLocks) +
                                ", good locks "            + std::to_string(nGoodLocks) +
                                ", PLL lock count "        + std::to_string(pllLockCnt) +
                                ", mmcm phase count = "    + std::to_string(phase) +
                                ", mmcm phase ns = "       + std::to_string(phaseNs) + "ns");
          }
          nBadLocks += 1;
        } else {
          if (!nextLockFound) {
            LOGGER->log_message(LogManager::INFO,
                                "[Using PLL lock method] Found next lock after " + std::to_string(i+1) + " shifts:"
                                " bad locks "            + std::to_string(nBadLocks) +
                                ", good locks "          + std::to_string(nGoodLocks) +
                                ", PLL lock count "      + std::to_string(pllLockCnt) +
                                ", mmcm phase count = "  + std::to_string(phase) +
                                ", mmcm phase ns = "     + std::to_string(phaseNs) + "ns");
            nextLockFound = true;
            nBadLocks     = 0;
          }
          nGoodLocks += 1;
        }  // end if (pllLockCnt < PLL_LOCK_READ_ATTEMPTS)
        if (nShiftsSinceLock == 1000) {
          LOGGER->log_message(LogManager::INFO,
                              "[Using PLL lock method] Finished 1000 shifts after first good lock:"
                              " bad locks "   + std::to_string(nBadLocks) +
                              ", good locks " + std::to_string(nGoodLocks));
          bestLockFound = true;
          if (doScan) {
            nextLockFound    = false;
            firstUnlockFound = false;
            nGoodLocks       = 0;
            nBadLocks        = 0;
            nShiftsSinceLock = 0;
          } else {
            break;
          }  // end if doScan
        }  // end if (nShiftsSinceLock == 1000)
      }  // end if (!firstUnlockFound)
    } else {
      if (shiftOutOfLockFirst && (pllLockCnt < PLL_LOCK_READ_ATTEMPTS) && !firstUnlockFound) {
        firstUnlockFound = true;
        LOGGER->log_message(LogManager::NOTICE,
                            "[Using original PLL method] Unlocked after "          + std::to_string(i+1) + "shifts:"
                            " mmcm phase count = "     + std::to_string(phase) +
                            ", mmcm phase ns = "       + std::to_string(phaseNs) + "ns"
                            ", pllLockCnt = "          + std::to_string(pllLockCnt) +
                            ", firstUnlockFound = "    + std::to_string(firstUnlockFound) +
                            ", shiftOutOfLockFirst = " + std::to_string(shiftOutOfLockFirst) );
      }

      if (pllLockCnt == PLL_LOCK_READ_ATTEMPTS) {
        if (!shiftOutOfLockFirst) {
          if (nGoodLocks == 50) { // maybe 200
            reversingForLock = true;
            LOGGER->log_message(LogManager::INFO,
                                "[Using original PLL method] 200 consecutive good PLL locks found:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns"
                                ", reversing scan direction");
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",0);
            writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",1);
          }
          if (reversingForLock && (nGoodLocks == 75)) { // maybe 300
            LOGGER->log_message(LogManager::INFO,
                                "[Using original PLL method] Best lock found after reversing:"
                                " phase count = " + std::to_string(phase) +
                                ", phase ns = "   + std::to_string(phaseNs) + "ns.");
            bestLockFound    = true;
            if (doScan) {
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_MANUAL_SHIFT_DIR",1);
              writeReg(la,"GEM_AMC.TTC.CTRL.PA_GTH_MANUAL_SHIFT_DIR",0);
              bestLockFound    = false;
              reversingForLock = false;
              nGoodLocks       = 0;
              nShiftsSinceLock = 0;
            } else {
              break;
            }
          }
        } else if (firstUnlockFound || !shiftOutOfLockFirst) {
          if (!nextLockFound) {
            LOGGER->log_message(LogManager::INFO,
                                "[Using original PLL method] Found next lock after " + std::to_string(i+1)+ " shifts:"
                                " bad locks "            + std::to_string(nBadLocks) +
                                ", good locks "          + std::to_string(nGoodLocks) +
                                ", mmcm phase count = "  + std::to_string(phase) +
                                ", mmcm phase ns = "     + std::to_string(phaseNs) + "ns");
            nextLockFound = true;
          }
          if (nShiftsSinceLock > 500) {
            bestLockFound = true;
            if (!doScan)
              break;
            nextLockFound    = false;
            firstUnlockFound = false;
            bestLockFound    = false;
            nGoodLocks       = 0;
            nShiftsSinceLock = 0;
          }
        } else {
          nGoodLocks += 1;
        }
      } else if (nextLockFound) {
        if (nShiftsSinceLock > 500) {
          bestLockFound = true;
          if (!doScan)
            break;
          nextLockFound    = false;
          firstUnlockFound = false;
          bestLockFound    = false;
          nGoodLocks       = 0;
          nShiftsSinceLock = 0;
        }
      } else {
        bestLockFound = false;
        nBadLocks += 1;
      }
    }
    if (nextLockFound) {
      nShiftsSinceLock += 1;
    }
    if (reversingForLock) {
      totalShiftCount -= 1;
    } else {
      totalShiftCount += 1;
    }
  }

  if (bestLockFound) {
    writeReg(la,"GEM_AMC.TTC.CTRL.MODULE_RESET", 0x1);
    std::stringstream msg;
    msg << "Lock was found: phase count " << phase << ", phase " << phaseNs << "ns";
    LOGGER->log_message(LogManager::INFO, msg.str());
  } else {
    std::stringstream errmsg;
    errmsg << "unable to find lock";
    LOGGER->log_message(LogManager::ERROR, errmsg.str());
    la->response->set_string("error","ttcMMCMPhaseShift: " + errmsg.str());
  }

  return;
} //End ttcMMCMPhaseShiftLocal()

void ttcMMCMPhaseShift(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    bool shiftOutOfLockFirst = request->get_word("shiftOutOfLockFirst");
    bool useBC0Locked        = request->get_word("useBC0Locked");
    bool doScan              = request->get_word("doScan");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    ttcMMCMPhaseShiftLocal(&la, shiftOutOfLockFirst, useBC0Locked, doScan);

    return;
} //End ttcMMCMPhaseShift()

extern "C" {
    const char *module_version_key = "amc v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memhub_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("amc", "checkPLLLock",           checkPLLLock);
        modmgr->register_method("amc", "getOHVFATMask",          getOHVFATMask);
        modmgr->register_method("amc", "getOHVFATMaskMultiLink", getOHVFATMaskMultiLink);
        modmgr->register_method("amc", "sbitReadOut",            sbitReadOut);
        modmgr->register_method("amc", "ttcMMCMPhaseShift",      ttcMMCMPhaseShift);
    }
}
