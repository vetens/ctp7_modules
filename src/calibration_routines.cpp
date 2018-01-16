#include <chrono>
#include <pthread.h>
#include "optohybrid.h"
#include <thread>
#include "vfat3.h"

void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch){
    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x0, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.OH_SELECT", ohN, la->response);
        if(ch>127)
        {
            //writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_SELECT", 0, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_GLOBAL_OR", 0x1, la->response);
        }
        else
        {
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_SELECT", ch, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_GLOBAL_OR", 0x0, la->response);
        }
    } //End v3 electronics behavior
    else if (iFWVersion == 1){ //v2b electronics behavior
        // Place holder
    } //End v2b electronics behavior

    return;
}

void ttcGenToggleLocal(localArgs * la, uint32_t ohN, bool enable){
    /*
     * v3  electronics: enable = true (false) ignore (take) ttc commands from backplane for this AMC
     * v2b electronics: enable = true (false) start (stop) the T1Controller for link ohN
     */

    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        if (enable){
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x1, la->response); //Internal, TTC cmds from backplane are ignored
        }
        else{
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x0, la->response); //External, TTC cmds from backplane
        }
    } //End v3 electronics behavior
    else if (iFWVersion == 1) { //v2b electronics behavior
        //base reg
        std::stringstream sstream;
        sstream<<ohN;
        std::string contBase = "GEM_AMC.OH.OH" + sstream.str() + ".T1Controller";

        if (enable){ //Start
            if ( !(readReg(la->rtxn, la->dbi, contBase + ".MONITOR"))){
                writeReg(la->rtxn, la->dbi, contBase + ".TOGGLE", 0x1, la->response);   //Enable
            }
        }
        else { //Stop
            if( readReg(la->rtxn, la->dbi, contBase + ".MONITOR")){
                writeReg(la->rtxn, la->dbi, contBase + ".TOGGLE", 0x0, la->response);   //Disable
            }
        }
    } //End v2b electronics behavior
    else {
        LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
    }

    return;
} //End ttcGenToggleLocal(...)

void ttcGenToggle(const RPCMsg *request, RPCMsg *response){
    /*
     * v3  electronics: enable = true (false) ignore (take) ttc commands from backplane for this AMC
     * v2b electronics: enable = true (false) start (stop) the T1Controller for link ohN
     */

    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    bool enable = request->get_word("enable");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    ttcGenToggleLocal(&la, ohN, enable);

    return;
} //End ttcGenToggle(...)

void ttcGenConfLocal(localArgs * la, uint32_t ohN, uint32_t mode, uint32_t type, uint32_t pulseDelay, uint32_t L1Ainterval, uint32_t nPulses, bool enable){
    /*
     * v3  electronics Behavior:
     *      pulseDelay (only for enable = true), delay between CalPulse and L1A
     *      L1Ainterval (only for enable = true), how often to repeat signals
     *      enable = true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
     * v2b electronics behavior:
     *      Configure the T1 controller
     *      mode: 0 (Single T1 signal),
     *            1 (CalPulse followed by L1A),
     *            2 (pattern)
     *      type (only for mode 0, type of T1 signal to send):
     *            0 L1A
     *            1 CalPulse
     *            2 Resync
     *            3 BC0
     *      pulseDelay (only for mode 1), delay between CalPulse and L1A
     *      L1Ainterval (only for mode 0,1), how often to repeat signals
     *      nPulses how many signals to send (0 is continuous)
     *      enable = true (false) start (stop) the T1Controller for link ohN
     */

    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.RESET", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_GAP", L1Ainterval, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_CALPULSE_TO_L1A_GAP", pulseDelay, la->response);
    } //End v3 electronics behavior
    else if (iFWVersion == 1){ //v2b electronics behavior
        //base reg
        std::stringstream sstream;
        sstream<<ohN;
        std::string contBase = "GEM_AMC.OH.OH" + sstream.str() + ".T1Controller";

        //reset the controller
        writeReg(la->rtxn,la->dbi,contBase + ".RESET",0x1,la->response);

        //Set the mode
        writeReg(la->rtxn,la->dbi,contBase + ".MODE",mode,la->response);
        LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for mode 0x%x (0x%x)",
                    ohN,mode,
                    readReg(la->rtxn, la->dbi, contBase + ".MODE")
                    )
                );

        if (mode == 0){
            writeReg(la->rtxn, la->dbi, contBase + ".TYPE", type, la->response);
            LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for type 0x%x (0x%x)",
                        ohN,type,
                        readReg(la->rtxn, la->dbi, contBase + ".TYPE")
                        )
                    );
        }
        if (mode == 1){
            writeReg(la->rtxn, la->dbi, contBase + ".DELAY", pulseDelay, la->response);
            LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for delay %i (%i)",
                        ohN,pulseDelay,
                        readReg(la->rtxn, la->dbi, contBase + ".DELAY")
                        )
                    );
        }
        if (mode != 2){
            writeReg(la->rtxn, la->dbi, contBase + ".INTERVAL", L1Ainterval, la->response);
            LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for interval %i (%i)",
                        ohN,L1Ainterval,
                        readReg(la->rtxn, la->dbi, contBase + ".INTERVAL")
                        )
                    );
        }

        writeReg(la->rtxn, la->dbi, contBase + ".NUMBER", nPulses, la->response);
        LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for nsignals %i (%i)",
                    ohN,nPulses,
                    readReg(la->rtxn, la->dbi, contBase + ".NUMBER")
                    )
                );

        //ttcGenToggleLocal(la, ohN, enable);
    } //End v2b electronics behavior
    else {
        LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
    }

    //start or stop
    ttcGenToggleLocal(la, ohN, enable);

    return;
}

void ttcGenConf(const RPCMsg *request, RPCMsg *response)
{
    /*
     * v3  electronics Behavior:
     *      pulseDelay (only for enable = true), delay between CalPulse and L1A
     *      L1Ainterval (only for enable = true), how often to repeat signals
     *      enable = true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
     * v2b electronics behavior:
     *      Configure the T1 controller
     *      mode: 0 (Single T1 signal),
     *            1 (CalPulse followed by L1A),
     *            2 (pattern)
     *      type (only for mode 0, type of T1 signal to send):
     *            0 L1A
     *            1 CalPulse
     *            2 Resync
     *            3 BC0
     *      pulseDelay (only for mode 1), delay between CalPulse and L1A
     *      L1Ainterval (only for mode 0,1), how often to repeat signals
     *      nPulses how many signals to send (0 is continuous)
     *      enable = true (false) start (stop) the T1Controller for link ohN
     */

    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t mode = request->get_word("mode");
    uint32_t type = request->get_word("type");
    uint32_t pulseDelay = request->get_word("pulseDelay");
    uint32_t L1Ainterval = request->get_word("L1Ainterval");
    uint32_t nPulses = request->get_word("nPulses");
    bool enable = request->get_word("enable");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    ttcGenConfLocal(&la, ohN, mode, type, pulseDelay, L1Ainterval, nPulses, enable);

    return;
}

void genScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, uint32_t ch, bool useCalPulse, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, bool useUltra, bool useExtTrig){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
        char regBuf[200];
        if( (notmask & goodVFATs) != notmask)
        {
            sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
            la->response->set_string("error",regBuf);
            return;
        }

        //Do we turn on the calpulse for the channel = ch?
        if(useCalPulse){
            if(ch >= 128){
                la->response->set_string("error","It doesn't make sense to calpulse all channels");
                return;
            }
            else{
                for(int vfatN = 0; vfatN < 24; vfatN++){
                    if((notmask >> vfatN) & 0x1){
                        sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                        writeReg(la->rtxn, la->dbi, regBuf, 0x1, la->response);
                        writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x1, la->response);
                    }
                }
            }
        }

        //Get addresses
        uint32_t scanDacAddr[24];
        uint32_t daqMonAddr[24];
        uint32_t l1CntAddr = getAddress(la->rtxn, la->dbi, "GEM_AMC.TTC.CMD_COUNTERS.L1A", la->response);
        for(int vfatN = 0; vfatN < 24; vfatN++)
        {
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str());
            scanDacAddr[vfatN] = getAddress(la->rtxn, la->dbi, regBuf, la->response);
            sprintf(regBuf,"GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN);
            daqMonAddr[vfatN] = getAddress(la->rtxn, la->dbi, regBuf, la->response);
        }

        //TTC Config
        if(useExtTrig){
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x0, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CTRL.CNT_RESET", 0x1, la->response);
        }
        else{
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", nevts, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1, la->response);
        }

        //Configure VFAT_DAQ_MONITOR
        dacMonConfLocal(la, ohN, ch);

        //Place VFATs out of slow control only mode
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x1, la->response);

        //Scan over DAC values
        for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep)
        {
            //Write the scan reg value
            for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
            {
                //writeRawAddress(scanDacAddr[vfatN], dacVal, la->response);
                writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str()), dacVal, la->response);
                //std::this_thread::sleep_for(std::chrono::microseconds(50)); //Seen some evidence that
            }

            //Reset and enable the VFAT_DAQ_MONITOR
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x1, la->response);

            //Start the triggers
            if(useExtTrig){
                writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CTRL.CNT_RESET", 0x1, la->response);
                writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x1, la->response);

                uint32_t l1aCnt = 0;
                while(l1aCnt < nevts){
                    l1aCnt = readRawAddress(l1CntAddr, la->response);
                    //std::this_thread::sleep_for(std::chrono::microseconds(50));
                    std::this_thread::sleep_for(std::chrono::microseconds(200));
                }

                writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x0, la->response);
                l1aCnt = readRawAddress(l1CntAddr, la->response);
            }
            else{
                writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_START", 0x1, la->response);
                if(readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.ENABLE")){ //TTC Commands from TTC.GENERATOR
                    while(readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_RUNNING")){
                        std::this_thread::sleep_for(std::chrono::microseconds(50));
                    }
                } //End TTC Commands from TTC.GENERATOR
            }


            for(int vfatN = 0; vfatN < 24; vfatN++){
                int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
                outData[idx] = readRawAddress(daqMonAddr[vfatN], la->response);

                if((notmask >> vfatN) & 0x1){
                    LOGGER->log_message(LogManager::DEBUG, stdsprintf("%s Value: %i; Readback Val: %i; Nhits: %i; Nev: %i; CFG_THR_ARM: %i",
                                 scanReg.c_str(),
                                 dacVal,
                                 readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str())),
                                 readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.CHANNEL_FIRE_COUNT",vfatN)),
                                 readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN)),
                                 readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_THR_ARM_DAC",ohN,vfatN,scanReg.c_str()))
                        )
                    );
                }
            }
        } //End Loop from dacMin to dacMax

        //If the calpulse for channel ch was turned on, turn it off
        if(useCalPulse){
            for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
            {
                sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                if(ch < 128){
                    writeReg(la->rtxn, la->dbi, regBuf, 0x0, la->response);
                    writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x0, la->response);
                }
            }
        }
    } //End v3 electronics behavior
    else if (iFWVersion == 1){ //v2b electronics behavior
        //Determine scanmode
        std::map<int, std::string> map_strKnownRegs; //Key -> scanmode; val -> register

        map_strKnownRegs[0] = "VThreshold1";
        map_strKnownRegs[1] = "VThreshold1PerChan";
        map_strKnownRegs[2] = "Latency";
        map_strKnownRegs[3] = "VCal";
        map_strKnownRegs[4] = "VThreshold1Trk";

        uint32_t scanmode = 1000;

        for (auto knownRegIter = map_strKnownRegs.begin(); knownRegIter != map_strKnownRegs.end(); ++knownRegIter){
            //Comparison code goes here
            if ( (*knownRegIter).second.compare(scanReg) == 0){
                scanmode = (*knownRegIter).first;
                break;
            }
        }

        //scanmode not understood
        if (scanmode == 1000){
            std::string strError = "scanReg: " + scanReg + " not understood.  Supported values are:\n";
            for (auto knownRegIter = map_strKnownRegs.begin(); knownRegIter != map_strKnownRegs.end(); ++knownRegIter){
                scanReg += ((*knownRegIter).second + "\n");
            }
            la->response->set_string("error",strError);
        }

        //Configure scan module
        uint32_t vfatN = 0;
        if (!useUltra){
            //If we are not performing an ultraScan, take the first non-masked VFAT
            for(int vfat=0; vfat<24; ++vfat){
                if((notmask >> vfat) & 0x1){
                    vfatN=vfat;
                    break;
                }
            }
        }

        configureScanModuleLocal(la, ohN, vfatN, scanmode, useUltra, mask, ch, nevts, dacMin, dacMax, dacStep);

        //Print scan configuration
        printScanConfigurationLocal(la, ohN, useUltra);

        //Do we turn on the calpulse for the channel = ch?
        uint32_t trimVal=0;
        if(useCalPulse){
            if(ch >= 128){
                la->response->set_string("error","It doesn't make sense to calpulse all channels");
                return;
            }
            else{
                for(int vfat=0; vfat<24; ++vfat){
                    if ( (notmask >> vfat) & 0x1){
                        trimVal = (0x3f & readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch)));
                        writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch),trimVal+64, la->response);
                    }
                }
            }
        }

        //Start scan configuration
        startScanModuleLocal(la, ohN, useUltra);

        //If the calpulse for channel ch was turned on, turn it off
        if(useCalPulse){
            for(int vfat=0; vfat<24; ++vfat){
                if ( (notmask >> vfat) & 0x1){
                    trimVal = (0x3f & readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch)));
                    writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch),trimVal, la->response);
                }
            }
        }

        //Get scan results
        getUltraScanResultsLocal(la, outData, ohN, nevts, dacMin, dacMax, dacStep);
    } //End v2b electronics behavior
    else {
        LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
    }

    return;
} //End genScanLocal(...)

void genScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t nevts = request->get_word("nevts");
    uint32_t ohN = request->get_word("ohN");
    uint32_t ch = request->get_word("ch");
    uint32_t mask = request->get_word("mask");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");
    bool useCalPulse = request->get_word("useCalPulse");
    std::string scanReg = request->get_string("scanReg");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }
    bool useExtTrig = request->get_word("useExtTrig");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*(dacMax-dacMin+1)/dacStep];
    genScanLocal(&la, outData, ohN, mask, ch, useCalPulse, nevts, dacMin, dacMax, dacStep, scanReg, useUltra, useExtTrig);
    response->set_word_array("data",outData,24*(dacMax-dacMin+1)/dacStep);

    return;
}

void sbitRateScanLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRate, uint32_t ohN, uint32_t maskOh, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, uint32_t waitTime){
    //Measures the SBIT rate seen by OHv3 ohN for the non-masked VFAT found in maskOh as a function of scanReg
    //It is assumed that all other VFATs are masked in the OHv3 via maskOh
    //Will scan from dacMin to dacMax in steps of dacStep
    //The x-values (e.g. scanReg values) will be stored in outDataDacVal
    //The y-valued (e.g. rate) will be stored in outDataTrigRate
    //Each measured point will take waitTime milliseconds (recommond between 1000->3000)
    //The measurement is performed for all channels (ch=128) or a specific channel (0 <= ch <= 127)

    char regBuf[200];
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
    if (iFWVersion < 3){ //v3 electronics behavior
        sprintf(regBuf,"SBIT Rate Scan is Presently only supported in V3 Electronics");
        la->response->set_string("error",regBuf);
        return;
    }

    //Hard code possible maskOh values and how they map to vfatN
    std::map<uint32_t,uint32_t> map_maskOh2vfatN;
    map_maskOh2vfatN[0xfffffe] = 0;
    map_maskOh2vfatN[0xfffffd] = 1;
    map_maskOh2vfatN[0xfffffb] = 2;
    map_maskOh2vfatN[0xfffff7] = 3;
    map_maskOh2vfatN[0xffffef] = 4;
    map_maskOh2vfatN[0xffffdf] = 5;
    map_maskOh2vfatN[0xffffbf] = 6;
    map_maskOh2vfatN[0xffff7f] = 7;
    map_maskOh2vfatN[0xfffeff] = 8;
    map_maskOh2vfatN[0xfffdff] = 9;
    map_maskOh2vfatN[0xfffbff] = 10;
    map_maskOh2vfatN[0xfff7ff] = 11;
    map_maskOh2vfatN[0xffefff] = 12;
    map_maskOh2vfatN[0xffdfff] = 13;
    map_maskOh2vfatN[0xffbfff] = 14;
    map_maskOh2vfatN[0xff7fff] = 15;
    map_maskOh2vfatN[0xfeffff] = 16;
    map_maskOh2vfatN[0xfdffff] = 17;
    map_maskOh2vfatN[0xfbffff] = 18;
    map_maskOh2vfatN[0xf7ffff] = 19;
    map_maskOh2vfatN[0xefffff] = 20;
    map_maskOh2vfatN[0xdfffff] = 21;
    map_maskOh2vfatN[0xbfffff] = 22;
    map_maskOh2vfatN[0x7fffff] = 23;

    //Determine vfatN based on input maskOh
    auto vfatNptr = map_maskOh2vfatN.find(maskOh);
    if( vfatNptr != map_maskOh2vfatN.end() ){
        sprintf(regBuf,"Input maskOh: %x not recgonized. Please make sure all but one VFAT is unmasked and then try again", maskOh);
        la->response->set_string("error",regBuf);
        return;
    }

    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    if( !( (goodVFATs >> (*vfatNptr) ) & 0x1 ) ){
        sprintf(regBuf,"The requested VFAT is not synced; goodVFATs: %x\t requested VFAT: %i; maskOh: %x", goodVFATs, (*vfatNptr), maskOh);
        la->response->set_string("error",regBuf);
        return;
    }

    //If ch!=128 store the original channel mask settings
    //Then mask all other channels except for channel ch
    std::map<uint32_t, uint32_t> map_chanOrigMask; //key -> reg addr; val -> reg value
    if( ch != 128){
        uint32_t chanMaskAddr[128];
        for(unsigned int chan=0; chan<128; ++chan){ //Loop Over All Channels
            uint32_t chMask = 1;
            if ( ch == chan){ //Do not mask the channel of interest
                chMask = 0;
            }

            //store the original channel mask
            sprintf(regBuf, "GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,(*vfatNptr),chan);
            chanMaskAddr[chan]=getAddress(la->rtxn, la->dbi, regBuf, la->response);
            map_chanOrigMask[chanMaskAddr[chan]]=readReg(la->rtxn, la->dbi, regBuf);   //We'll write this by address later

            //write the new channel mask
            writeRawAddress(chanMaskAddr[chan], chMask, la->response);
        } //End Loop Over all Channels
    } //End Case: Measuring Rate for 1 channel

    //Get the scanAddress
    sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,(*vfatNptr),scanReg.c_str());
    uint32_t scanDacAddr = getAddress(la->rtxn, la->dbi, regBuf, la->response);

    //Get the OH Rate Monitor Address
    sprintf(regBuf,"GEM_AMC.TRIGGER.OH%i.TRIGGER_RATE",ohN);
    uint32_t ohTrigRateAddr = getAddress(la->rtxn, la->dbi, regBuf, la->response);

    //Store the original OH  VFAT Mask, and then reset it
    sprintf(regBuf,"GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK"%(ohN))
    uint32_t ohVFATMaskAddr = getAddress(la->rtxn, la->dbi, regBuf, la->response);
    uint32_t maskOhOrig = readRawAddress(ohVFATMaskAddr, la->response);   //We'll write this later
    writeRawAddress(ohVFATMaskAddr, maskOh, la->response);

    //Take the VFATs out of slow control only mode
    writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x1, la->response);

    //Place chip in run mode
    //sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,(*vfatNptr));
    //uint32_t runAddr = getAddress(la->rtxn, la->dbi, regBuf, la->response);
    //writeRawAddress(runAddr, 0x1, la->response);

    //Loop from dacMin to dacMax in steps of dacStep
    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
        writeRawAddress(scanDacAddr, dacVal, la->response);
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));

        int idx = (dacVal-dacMin)/dacStep;
        outDataDacVal[idx] = dacVal;
        outDataTrigRate[idx] = readRawAddress(ohTrigRateAddr, la->response);
    } //End Loop from dacMin to dacMax

    //Take chip out of run mode
    //writeRawAddress(runAddr, 0x0, la->response);

    //Restore the original channel masks if specific channel was requested
    if( ch != 128){
        for(auto chanPtr = map_chanOrigMask.begin(); chanPtr != map_chanOrigMask.end(); ++chanPtr){
            writeRawAddress( (*chanPtr).first, (*chanPtr).second, la->response);
        }
    } //End restore original channel masks if specific channel was requested

    //Restore the original maskOh
    writeRawAddress(ohVFATMaskAddr, maskOhOrig, la->response);

    return;
} //End sbitRateScanLocal(...)

void sbitRateScan(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t maskOh = request->get_word("maskOh");
    uint32_t ch = request->get_word("ch");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");
    std::string scanReg = request->get_string("scanReg");
    uint32_t waitTime = request->get_word("waitTime");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outDataDacVal[(dacMax-dacMin+1)/dacStep];
    uint32_t outDataTrigRate[(dacMax-dacMin+1)/dacStep];
    sbitRateScanLocal(&la, outDataDacVal, outDataTrigRate, ohN, maskOh, ch, dacMin, dacMax, dacStep, scanReg, waitTime);

    response->set_word_array("data_dacVal", outDataDacVal, (dacMax-dacMin+1)/dacStep);
    response->set_word_array("data_trigRate", outDataTrigRate, (dacMax-dacMin+1)/dacStep);

    return;
} //End sbitRateScan(...)

void genChannelScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t nevts = request->get_word("nevts");
    uint32_t ohN = request->get_word("ohN");
    uint32_t mask = request->get_word("mask");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");
    uint32_t useCalPulse = request->get_word("useCalPulse");
    uint32_t useExtTrig = request->get_word("useExtTrig");
    std::string scanReg = request->get_string("scanReg");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[128*24*(dacMax-dacMin+1)/dacStep];
    for(uint32_t ch = 0; ch < 128; ch++)
    {
        genScanLocal(&la, &(outData[ch*24*(dacMax-dacMin+1)/dacStep]), ohN, mask, ch, useCalPulse, nevts, dacMin, dacMax, dacStep, scanReg, useUltra, useExtTrig);
    }
    response->set_word_array("data",outData,24*128*(dacMax-dacMin+1)/dacStep);

    return;
}

extern "C" {
    const char *module_version_key = "calibration_routines v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memsvc_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("calibration_routines", "genScan", genScan);
        modmgr->register_method("calibration_routines", "genChannelScan", genChannelScan);
        modmgr->register_method("calibration_routines", "sbitRateScan", sbitRateScan);
        modmgr->register_method("calibration_routines", "ttcGenConf", ttcGenConf);
        modmgr->register_method("calibration_routines", "ttcGenToggle", ttcGenToggle);
    }
}
