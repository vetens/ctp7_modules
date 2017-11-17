#include <pthread.h>
#include "optohybrid.h"
#include "vfat3.h"

void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch){
    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.OH_SELECT", ohN, la->response);
        if(ch==128)
        {
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_SELECT", 0, la->response);
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

void genScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, uint32_t ch, bool useCalPulse, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, bool useUltra){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    //Get firmware version
    int iFWVersion = readReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");

    if (iFWVersion == 3){ //v3 electronics behavior
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", nevts, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1, la->response);
        dacMonConfLocal(la, ohN, ch);
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
                    }
                }
            }
        }

        uint32_t scanDacAddr[24];
        uint32_t daqMonAddr[24];

        for(int vfatN = 0; vfatN < 24; vfatN++)
        {
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str());
            scanDacAddr[vfatN] = getAddress(la->rtxn, la->dbi, regBuf, la->response);
            sprintf(regBuf,"GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN);
            daqMonAddr[vfatN] = getAddress(la->rtxn, la->dbi, regBuf, la->response);

            //This shouldn't be done here since it will be written every channel
            /*if((notmask >> vfatN) & 0x1)
            {
                sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,vfatN);
                writeReg(la->rtxn, la->dbi, regBuf, 0x1, la->response);
            }*/
        }

        for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep)
        {
            for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
            {
                //writeRawAddress(scanDacAddr[vfatN], dacVal, la->response);
                writeReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str()), dacVal, la->response);
            }
            writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
            writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_START", 0x1, la->response);
            bool running = true;
            if(readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.ENABLE")){ //TTC Commands from TTC.GENERATOR
                while(running) running = readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_RUNNING");
            } //End TTC Commands from TTC.GENERATOR
            else { //TTC Commands from Backplane
                for(int vfatN = 0; vfatN < 24; vfatN++){
                    if((notmask >> vfatN) & 0x1){
                        uint32_t currentEvtNum = 0;
                        while(running){
                            currentEvtNum = readReg(la->rtxn, la->dbi, stdsprintf("GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN));
                            running = (currentEvtNum <= nevts);
                            if( currentEvtNum % 100 ){
                                LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%d: dacVal: %d; Trigger Recieved, L1A Num: %d",ohN, dacVal, readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.CMD_COUNTERS.L1A")));
                            }
                        }
                        break;
                    }
                }
            } //End TTC Commands from Backplane

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
                //sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,vfatN);
                //writeReg(la->rtxn, la->dbi, regBuf, 0x0, la->response);
                sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                if(ch < 128) writeReg(la->rtxn, la->dbi, regBuf, 0x0, la->response);
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

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*(dacMax-dacMin+1)/dacStep];
    genScanLocal(&la, outData, ohN, mask, ch, useCalPulse, nevts, dacMin, dacMax, dacStep, scanReg, useUltra);
    response->set_word_array("data",outData,24*(dacMax-dacMin+1)/dacStep);

    return;
}

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
    std::string scanReg = request->get_string("scanReg");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[128*24*(dacMax-dacMin+1)/dacStep];
    for(uint32_t ch = 0; ch < 128; ch++)
    {
        genScanLocal(&la, &(outData[ch*24*(dacMax-dacMin+1)/dacStep]), ohN, mask, ch, useCalPulse, nevts, dacMin, dacMax, dacStep, scanReg, useUltra);
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
        modmgr->register_method("calibration_routines", "genChannelScan", genScan);
        modmgr->register_method("calibration_routines", "ttcGenConf", ttcGenConf);
        modmgr->register_method("calibration_routines", "ttcGenToggle", ttcGenToggle);
    }
}
