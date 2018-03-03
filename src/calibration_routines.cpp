/*! \file calibration_routines.cpp
 *  \brief Calibration routines
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brin Dorney <brian.l.dorney@cern.ch>
 */

#include <chrono>
#include <pthread.h>
#include "optohybrid.h"
#include <thread>
#include "vfat3.h"

/*! \fn unsigned int fw_version_check(const char* caller_name, localArgs *la)
 *  \brief Returns AMC FW version
 *  in case FW version is below 3.X sets an error string in response
 *  \param caller_name Name of methods which called the FW version check
 *  \param la Local arguments structure
 */
unsigned int fw_version_check(const char* caller_name, localArgs *la)
{
    int iFWVersion = readReg(la, "GEM_AMC.GEM_SYSTEM.RELEASE.MAJOR");
    char regBuf[200];
    if (iFWVersion < 3){ //v2b electronics behavior
        sprintf(regBuf,"%s is Presently only supported in V3 Electronics", caller_name);
        la->response->set_string("error",regBuf);
    }
    return iFWVersion;
}

/*! \fn std::unordered_map<uint32_t, uint32_t> setSingleChanMask(int ohN, int vfatN, unsigned int ch, localArgs *la)
 *  \brief Unmask the channel of interest and masks all the other
 *  \param ohN Optical link number
 *  \param vfatN VFAT position 
 *  \param ch Channel of interest
 *  \param la Local arguments structure
 *  \return Original channel mask in a form of an unordered map <chanMaskAddr, mask>
 */
std::unordered_map<uint32_t, uint32_t> setSingleChanMask(int ohN, int vfatN, unsigned int ch, localArgs *la)
{
    char regBuf[200];
    std::unordered_map<uint32_t, uint32_t> map_chanOrigMask; //key -> reg addr; val -> reg value
    uint32_t chanMaskAddr;
    for(unsigned int chan=0; chan<128; ++chan){ //Loop Over All Channels
        uint32_t chMask = 1;
        if ( ch == chan){ //Do not mask the channel of interest
            chMask = 0;
        }
        //store the original channel mask
        sprintf(regBuf, "GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,vfatN,chan);
        chanMaskAddr=getAddress(la, regBuf);
        map_chanOrigMask[chanMaskAddr]=readReg(la, regBuf);   //We'll write this by address later

        //write the new channel mask
        writeRawAddress(chanMaskAddr, chMask, la->response);
    } //End Loop Over all Channels
    return map_chanOrigMask;
}

/*! \fn void applyChanMask(std::unordered_map<uint32_t, uint32_t> map_chanOrigMask, localArgs *la)
 *  \brief Applies channel mask
 *  \param map_chanOrigMask Original channel mask as obtained from setSingleChanMask mehod
 *  \param la Local arguments structure
 */
void applyChanMask(std::unordered_map<uint32_t, uint32_t> map_chanOrigMask, localArgs *la)
{
    for(auto chanPtr = map_chanOrigMask.begin(); chanPtr != map_chanOrigMask.end(); ++chanPtr){
        writeRawAddress( (*chanPtr).first, (*chanPtr).second, la->response);
    }
}

/*! \fn void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch)
 *  \brief Configures DAQ monitor. Local version only
 *  \param la Local arguments structure
 *  \param ohN Optical link number
 *  \param ch Channel of interest
 */
void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch)
{
    //Check the firmware version
    if (fw_version_check("dacMonConf", la) == 3){ //v3 electronics behavior
        writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x0);
        writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1);
        writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.OH_SELECT", ohN);
        if(ch>127)
        {
            //writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_SELECT", 0);
            writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_GLOBAL_OR", 0x1);
        }
        else
        {
            writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_SELECT", ch);
            writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.VFAT_CHANNEL_GLOBAL_OR", 0x0);
        }
    } //End v3 electronics behavior

    return;
}

/*! \fn void ttcGenToggleLocal(localArgs * la, uint32_t ohN, bool enable)
 *  \brief Enables the TTC commands from backplane. Local callable version of ttcGenToggle
 *
 *  * v3  electronics: enable = true (false) ignore (take) ttc commands from backplane for this AMC
 *  * v2b electronics: enable = true (false) start (stop) the T1Controller for link ohN
 *
 *  \param la Local arguments structure
 *  \param ohN Optical link
 *  \param enable See detailed mehod description
 */
void ttcGenToggleLocal(localArgs * la, uint32_t ohN, bool enable)
{
    //Check firmware version
    switch(fw_version_check("ttcGenToggle", la)) {
        case 3: //v3 electronics behavior
        {
            if (enable){
                writeReg(la, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x1); //Internal, TTC cmds from backplane are ignored
            }
            else{
                writeReg(la, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x0); //External, TTC cmds from backplane
            }
        }//End v3 electronics behavior
        case 1: //v2b electronics behavior
        {
            //base reg
            std::stringstream sstream;
            sstream<<ohN;
            std::string contBase = "GEM_AMC.OH.OH" + sstream.str() + ".T1Controller";

            if (enable){ //Start
                if ( !(readReg(la, contBase + ".MONITOR"))){
                    writeReg(la, contBase + ".TOGGLE", 0x1);   //Enable
                }
            }
            else { //Stop
                if( readReg(la, contBase + ".MONITOR")){
                    writeReg(la, contBase + ".TOGGLE", 0x0);   //Disable
                }
            }
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
        }
    }

    return;
} //End ttcGenToggleLocal(...)

/*! \fn void ttcGenToggle(const RPCMsg *request, RPCMsg *response)
 *  \brief Enables the TTC commands from backplane
 *
 *  * v3  electronics: enable = true (false) ignore (take) ttc commands from backplane for this AMC
 *  * v2b electronics: enable = true (false) start (stop) the T1Controller for link ohN
 *
 *  \param request RPC request message
 *  \param response RPC response message
 */
void ttcGenToggle(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    bool enable = request->get_word("enable");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    ttcGenToggleLocal(&la, ohN, enable);

    return;
} //End ttcGenToggle(...)

/*! \fn void ttcGenConfLocal(localArgs * la, uint32_t ohN, uint32_t mode, uint32_t type, uint32_t pulseDelay, uint32_t L1Ainterval, uint32_t nPulses, bool enable)
 *  \brief Configures TTC generator. Local callable version of ttcGenConf
 *
 *  1. v3  electronics behavior:
 *    * pulseDelay (only for enable = true), delay between CalPulse and L1A
 *    * L1Ainterval (only for enable = true), how often to repeat signals
 *    * enable = true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
 *  2. v2b electronics behavior:
 *    * Configure the T1 controller
 *    * mode: 
 *      * 0 (Single T1 signal),
 *      * 1 (CalPulse followed by L1A),
 *      * 2 (pattern)
 *    * type (only for mode 0, type of T1 signal to send):
 *      * 0 L1A
 *      * 1 CalPulse
 *      * 2 Resync
 *      * 3 BC0
 *    * pulseDelay (only for mode 1), delay between CalPulse and L1A
 *    * L1Ainterval (only for mode 0,1), how often to repeat signals
 *    * nPulses how many signals to send (0 is continuous)
 *    * enable = true (false) start (stop) the T1Controller for link ohN
 *
 *  \param la Local arguments structure
 *  \param ohN Optical link
 *  \param mode T1 controller mode
 *  \param type Type of T1 signal to send
 *  \param pulseDelay Delay between CalPulse and L1A
 *  \param L1Ainterval How often to repeat signals (only for enable = true)
 *  \param nPulses Number of calibration pulses to generate
 *  \param enable If true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
 */
void ttcGenConfLocal(localArgs * la, uint32_t ohN, uint32_t mode, uint32_t type, uint32_t pulseDelay, uint32_t L1Ainterval, uint32_t nPulses, bool enable)
{
    //Check firmware version
    switch(fw_version_check("ttcGenToggle", la)) {
        case 3: //v3 electronics behavior
        {
            writeReg(la, "GEM_AMC.TTC.GENERATOR.RESET", 0x1);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_GAP", L1Ainterval);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_CALPULSE_TO_L1A_GAP", pulseDelay);
        }//End v3 electronics behavior
        case 1: //v2b electronics behavior
        {
            //base reg
            std::stringstream sstream;
            sstream<<ohN;
            std::string contBase = "GEM_AMC.OH.OH" + sstream.str() + ".T1Controller";

            //reset the controller
            writeReg(la, contBase + ".RESET",0x1);

            //Set the mode
            writeReg(la, contBase + ".MODE",mode);
            LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for mode 0x%x (0x%x)",
                        ohN,mode,
                        readReg(la, contBase + ".MODE")
                        )
                    );

            if (mode == 0){
                writeReg(la, contBase + ".TYPE", type);
                LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for type 0x%x (0x%x)",
                            ohN,type,
                            readReg(la, contBase + ".TYPE")
                            )
                        );
            }
            if (mode == 1){
                writeReg(la, contBase + ".DELAY", pulseDelay);
                LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for delay %i (%i)",
                            ohN,pulseDelay,
                            readReg(la, contBase + ".DELAY")
                            )
                        );
            }
            if (mode != 2){
                writeReg(la, contBase + ".INTERVAL", L1Ainterval);
                LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for interval %i (%i)",
                            ohN,L1Ainterval,
                            readReg(la, contBase + ".INTERVAL")
                            )
                        );
            }

            writeReg(la, contBase + ".NUMBER", nPulses);
            LOGGER->log_message(LogManager::DEBUG, stdsprintf("OH%i : Configuring T1 Controller for nsignals %i (%i)",
                        ohN,nPulses,
                        readReg(la, contBase + ".NUMBER")
                        )
                    );

            //ttcGenToggleLocal(la, ohN, enable);
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
        }
    }

    //start or stop
    ttcGenToggleLocal(la, ohN, enable);

    return;
}

/*! \fn void ttcGenConf(const RPCMsg *request, RPCMsg *response)
 *  \brief Configures TTC generator
 *
 *  * v3  electronics behavior:
 *  *      pulseDelay (only for enable = true), delay between CalPulse and L1A
 *  *      L1Ainterval (only for enable = true), how often to repeat signals
 *  *      enable = true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
 *  * v2b electronics behavior:
 *  *      Configure the T1 controller
 *  *      mode: 0 (Single T1 signal),
 *  *            1 (CalPulse followed by L1A),
 *  *            2 (pattern)
 *  *      type (only for mode 0, type of T1 signal to send):
 *  *            0 L1A
 *  *            1 CalPulse
 *  *            2 Resync
 *  *            3 BC0
 *  *      pulseDelay (only for mode 1), delay between CalPulse and L1A
 *  *      L1Ainterval (only for mode 0,1), how often to repeat signals
 *  *      nPulses how many signals to send (0 is continuous)
 *  *      enable = true (false) start (stop) the T1Controller for link ohN
 *
 *  \param request RPC request message
 *  \param response RPC response message
 */
void ttcGenConf(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
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

/*! \fn void genScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, uint32_t ch, bool useCalPulse, bool currentPulse, uint32_t calScaleFactor, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, bool useUltra, bool useExtTrig)
 *  \brief Generic calibration routine. Local callable version of genScan
 *  \param la Local arguments structure
 *  \param outData pointer to the results of the scan
 *  \param ohN Optical link
 *  \param mask VFAT mask
 *  \param ch Channel of interest
 *  \param useCalPulse Use  calibration pulse if true
 *  \param currentPulse Selects whether to use current or volage pulse
 *  \param calScaleFactor 
 *  \param nevts Number of events per calibration point
 *  \param dacMin Minimal value of scan variable
 *  \param dacMax Maximal value of scan variable
 *  \param dacStep Scan variable change step
 *  \param scanReg DAC register to scan over name 
 *  \param useUltra Set to 1 in order to use the ultra scan
 *  \param useExtTrig Set to 1 in order to use the backplane triggers
 */
void genScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, uint32_t ch, bool useCalPulse, bool currentPulse, uint32_t calScaleFactor, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, bool useUltra, bool useExtTrig)
{
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    //Check firmware version
    switch(fw_version_check("ttcGenToggle", la)) {
        case 3: //v3 electronics behavior
        {
            uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
            char regBuf[200];
            if( (notmask & goodVFATs) != notmask)
            {
                sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
                la->response->set_string("error",regBuf);
                return;
            }

            if (currentPulse && calScaleFactor > 3){
                sprintf(regBuf,"Bad value for CFG_CAL_FS: %x, Possible values are {0b00, 0b01, 0b10, 0b11}. Exiting.",calScaleFactor);
                la->response->set_string("error",regBuf);
                return;
            }

            //Do we turn on the calpulse for the channel = ch?
            if(useCalPulse){
                if(ch >= 128){ //Case: OR of all channels
                    la->response->set_string("error","It doesn't make sense to calpulse all channels");
                    return;
                } //End Case: OR of all channels
                else{ //Case: Pulse a specific channel
                    for(int vfatN = 0; vfatN < 24; vfatN++){ //Loop over all VFATs
                        if((notmask >> vfatN) & 0x1){ //End VFAT is not masked
                            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                            writeReg(la, regBuf, 0x1);

                            if(currentPulse){ //Case: cal mode current injection
                                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x2);

                                //Set cal current pulse scale factor. Q = CAL DUR[s] * CAL DAC * 10nA * CAL FS[%] (00 = 25%, 01 = 50%, 10 = 75%, 11 = 100%)
                                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_FS", ohN, vfatN), calScaleFactor);
                                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_DUR", ohN, vfatN), 0x0);
                            } //End Case: cal mode current injection
                            else { //Case: cal mode voltage injection
                                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x1);
                            } //Case: cal mode voltage injection
                        } //End VFAT is not masked
                    } //End Loop over all VFATs
                } //End Case: Pulse a specific channel
            } //End use calibration pulse

            //Get addresses
            uint32_t scanDacAddr[24];
            uint32_t daqMonAddr[24];
            uint32_t l1CntAddr = getAddress(la, "GEM_AMC.TTC.CMD_COUNTERS.L1A");
            for(int vfatN = 0; vfatN < 24; vfatN++)
            {
                sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str());
                scanDacAddr[vfatN] = getAddress(la, regBuf);
                sprintf(regBuf,"GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN);
                daqMonAddr[vfatN] = getAddress(la, regBuf);
            }

            //TTC Config
            if(useExtTrig){
                writeReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x0);
                writeReg(la, "GEM_AMC.TTC.CTRL.CNT_RESET", 0x1);
            }
            else{
                writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", nevts);
                writeReg(la, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1);
            }

            //Configure VFAT_DAQ_MONITOR
            dacMonConfLocal(la, ohN, ch);

            //Scan over DAC values
            for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep)
            {
                //Write the scan reg value
                for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
                {
                    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str()), dacVal);
                }

                //Reset and enable the VFAT_DAQ_MONITOR
                writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1);
                writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x1);

                //Start the triggers
                if(useExtTrig){
                    writeReg(la, "GEM_AMC.TTC.CTRL.CNT_RESET", 0x1);
                    writeReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x1);

                    uint32_t l1aCnt = 0;
                    while(l1aCnt < nevts){
                        l1aCnt = readRawAddress(l1CntAddr, la->response);
                        std::this_thread::sleep_for(std::chrono::microseconds(200));
                    }

                    writeReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE", 0x0);
                    l1aCnt = readRawAddress(l1CntAddr, la->response);
                }
                else{
                    writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_START", 0x1);
                    if(readReg(la, "GEM_AMC.TTC.GENERATOR.ENABLE")){ //TTC Commands from TTC.GENERATOR
                        while(readReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_RUNNING")){
                            std::this_thread::sleep_for(std::chrono::microseconds(50));
                        }
                    } //End TTC Commands from TTC.GENERATOR
                }


                for(int vfatN = 0; vfatN < 24; vfatN++){
                    if ( !( (notmask >> vfatN) & 0x1)) continue;

                    int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
                    outData[idx] = readRawAddress(daqMonAddr[vfatN], la->response);

                    LOGGER->log_message(LogManager::DEBUG, stdsprintf("%s Value: %i; Readback Val: %i; Nhits: %i; Nev: %i; CFG_THR_ARM: %i",
                                 scanReg.c_str(),
                                 dacVal,
                                 readReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str())),
                                 readReg(la, stdsprintf("GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.CHANNEL_FIRE_COUNT",vfatN)),
                                 readReg(la, stdsprintf("GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN)),
                                 readReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_THR_ARM_DAC",ohN,vfatN,scanReg.c_str()))
                        )
                    );
                } //End Loop over vfats
            } //End Loop from dacMin to dacMax

            //If the calpulse for channel ch was turned on, turn it off
            if(useCalPulse){
                for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
                {
                    sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                    if(ch < 128){
                        writeReg(la, regBuf, 0x0);
                        writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x0);
                    }
                }
            }
        }//End v3 electronics behavior
        case 1: //v2b electronics behavior
        {
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
                            trimVal = (0x3f & readReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch)));
                            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch),trimVal+64);
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
                        trimVal = (0x3f & readReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch)));
                        writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFATS.VFAT%i.VFATChannels.ChanReg%i",ohN,vfat,ch),trimVal);
                    }
                }
            }

            //Get scan results
            getUltraScanResultsLocal(la, outData, ohN, nevts, dacMin, dacMax, dacStep);
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major!");
        }
    }

    return;
} //End genScanLocal(...)

/*! \fn void genScan(const RPCMsg *request, RPCMsg *response)
 *  \brief Generic calibration routine
 *  \param request RPC request message
 *  \param response RPC response message
 */
void genScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
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
    bool currentPulse = request->get_word("currentPulse");
    uint32_t calScaleFactor = request->get_word("calScaleFactor");
    std::string scanReg = request->get_string("scanReg");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }
    bool useExtTrig = request->get_word("useExtTrig");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*(dacMax-dacMin+1)/dacStep];
    genScanLocal(&la, outData, ohN, mask, ch, useCalPulse, currentPulse, calScaleFactor, nevts, dacMin, dacMax, dacStep, scanReg, useUltra, useExtTrig);
    response->set_word_array("data",outData,24*(dacMax-dacMin+1)/dacStep);

    return;
}

/*! \fn void sbitRateScanLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRate, uint32_t ohN, uint32_t maskOh, bool invertVFATPos, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, uint32_t waitTime)
 *  \brief SBIT rate scan. Local version of sbitRateScan
 *
 *  * Measures the SBIT rate seen by OHv3 ohN for the non-masked VFAT found in maskOh as a function of scanReg
 *  * It is assumed that all other VFATs are masked in the OHv3 via maskOh
 *  * Will scan from dacMin to dacMax in steps of dacStep
 *  * The x-values (e.g. scanReg values) will be stored in outDataDacVal
 *  * The y-valued (e.g. rate) will be stored in outDataTrigRate
 *  * Each measured point will take waitTime milliseconds (recommond between 1000->3000)
 *  * The measurement is performed for all channels (ch=128) or a specific channel (0 <= ch <= 127)
 *  * invertVFATPos is for FW backwards compatiblity; if true then the vfatN =  23 - map_maskOh2vfatN[maskOh]
 *  
 *  \param la Local arguments structure
 *  \param outDataDacVal 
 *  \param outDataTrigRate
 *  \param ohN Optohybrid optical link number 
 *  \param maskOh VFAT mask, should only have one unmasked chip
 *  \param invertVFATPos is for FW backwards compatiblity; if true then the vfatN =  23 - map_maskOh2vfatN[maskOh]
 *  \param ch Channel of interest 
 *  \param dacMin Minimal value of scan variable
 *  \param dacMax Maximal value of scan variable
 *  \param dacStep Scan variable change step
 *  \param scanReg DAC register to scan over name 
 *  \waitTime Measurement duration per point
 */
void sbitRateScanLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRate, uint32_t ohN, uint32_t maskOh, bool invertVFATPos, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, uint32_t waitTime)
{
    char regBuf[200];
    if (fw_version_check("SBIT Rate Scan", la) < 3) return;

    //Hard code possible maskOh values and how they map to vfatN
    std::unordered_map<uint32_t,uint32_t> map_maskOh2vfatN;
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
    if( vfatNptr == map_maskOh2vfatN.end() ){
        sprintf(regBuf,"Input maskOh: %x not recgonized. Please make sure all but one VFAT is unmasked and then try again", maskOh);
        la->response->set_string("error",regBuf);
        return;
    }
    uint32_t vfatN = (invertVFATPos) ? 23 - (*vfatNptr).second : (*vfatNptr).second;

    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    if( !( (goodVFATs >> vfatN ) & 0x1 ) ){
        sprintf(regBuf,"The requested VFAT is not synced; goodVFATs: %x\t requested VFAT: %i; maskOh: %x", goodVFATs, vfatN, maskOh);
        la->response->set_string("error",regBuf);
        return;
    }

    //If ch!=128 store the original channel mask settings
    //Then mask all other channels except for channel ch
    std::unordered_map<uint32_t, uint32_t> map_chanOrigMask; //key -> reg addr; val -> reg value
    if( ch != 128) map_chanOrigMask = setSingleChanMask(ohN,vfatN,ch,la);

    //Get the OH Rate Monitor Address
    sprintf(regBuf,"GEM_AMC.TRIGGER.OH%i.TRIGGER_RATE",ohN);
    uint32_t ohTrigRateAddr = getAddress(la, regBuf);

    //Store the original OH VFAT Mask, and then reset it
    sprintf(regBuf,"GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK",ohN);
    uint32_t ohVFATMaskAddr = getAddress(la, regBuf);
    uint32_t maskOhOrig = readRawAddress(ohVFATMaskAddr, la->response);   //We'll write this later
    writeRawAddress(ohVFATMaskAddr, maskOh, la->response);

    //Take the VFATs out of slow control only mode
    writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

    //Loop from dacMin to dacMax in steps of dacStep
    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
        //writeRawAddress(scanDacAddr, dacVal);
        sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str());
        writeReg(la, regBuf, dacVal);
        std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));

        int idx = (dacVal-dacMin)/dacStep;
        outDataDacVal[idx] = dacVal;
        outDataTrigRate[idx] = readRawAddress(ohTrigRateAddr, la->response);
    } //End Loop from dacMin to dacMax

    //Restore the original channel masks if specific channel was requested
    if( ch != 128) applyChanMask(map_chanOrigMask, la);

    //Restore the original maskOh
    writeRawAddress(ohVFATMaskAddr, maskOhOrig, la->response);

    return;
} //End sbitRateScanLocal(...)

/*! \fn void sbitRateScanParallelLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRatePerVFAT, uint32_t *outDataTrigRateOverall, uint32_t ohN, uint32_t vfatmask, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg)
 *  \brief Parallel SBIT rate scan. Local version of sbitRateScan
 *
 *  * Measures the SBIT rate seen by OHv3 ohN for the non-masked VFATs defined in vfatmask as a function of scanReg
 *  * Will scan from dacMin to dacMax in steps of dacStep
 *  * The x-values (e.g. scanReg values) will be stored in outDataDacVal
 *  * For each VFAT the y-valued (e.g. rate) will be stored in outDataTrigRatePerVFAT
 *  * For the overall y-value (e.g. rate) will be stored in outDataTrigRateOverall
 *  * Each measured point will take one second
 *  * The measurement is performed for all channels (ch=128) or a specific channel (0 <= ch <= 127)
 *   
 *  \param la Local arguments structure
 *  \param outDataDacVal 
 *  \param outDataTrigRatePerVFAT
 *  \param outDataTrigRateOverall
 *  \param ohN Optohybrid optical link number 
 *  \param vfatMask VFAT mask 
 *  \param ch Channel of interest 
 *  \param dacMin Minimal value of scan variable
 *  \param dacMax Maximal value of scan variable
 *  \param dacStep Scan variable change step
 *  \param scanReg DAC register to scan over name 
 */
void sbitRateScanParallelLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRatePerVFAT, uint32_t *outDataTrigRateOverall, uint32_t ohN, uint32_t vfatmask, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg)
{
    char regBuf[200];

    //Are we using v3 electronics?
    if (fw_version_check("SBIT Rate Scan", la) < 3) return;

    //Check if vfats are sync'd
    uint32_t notmask = ~vfatmask & 0xFFFFFF;
    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    if( (notmask & goodVFATs) != notmask){
        sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        la->response->set_string("error",regBuf);
        return;
    }

    //If ch!=128 store the original channel mask settings
    //Then mask all other channels except for channel ch
    std::unordered_map<uint32_t, uint32_t> map_chanOrigMask[24]; //key -> reg addr; val -> reg value
    if( ch != 128){
        for(int vfat=0; vfat<24; ++vfat){
            //Skip this vfat if it's masked
            if ( !( (notmask >> vfat) & 0x1)) continue;
            map_chanOrigMask[vfat] = setSingleChanMask(ohN,vfat,ch,la);
        } //End loop over all vfats
    } //End Case: Measuring Rate for 1 channel

    //Get the SBIT Rate Monitor Address
    uint32_t ohTrigRateAddr[25]; //idx 0->23 VFAT counters; idx 24 overall rate
    sprintf(regBuf,"GEM_AMC.TRIGGER.OH%i.TRIGGER_RATE",ohN);
    ohTrigRateAddr[24] = getAddress(la, regBuf);
    for(int vfat=0; vfat<24; ++vfat){
        sprintf(regBuf,"GEM_AMC.OH.OH%i.TRIG.CNT.VFAT%i_SBITS",ohN,vfat);
        ohTrigRateAddr[vfat] = getAddress(la, regBuf);
    } //End Loop over all VFATs

    //Take the VFATs out of slow control only mode
    writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

    //Prep the SBIT counters
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.TRIG.CNT.SBIT_CNT_PERSIST",ohN), 0x0); //reset all counters after SBIT_CNT_TIME_MAX
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.TRIG.CNT.SBIT_CNT_TIME_MAX",ohN), 0x02638e98); //count for 1 second

    //Loop from dacMin to dacMax in steps of dacStep
    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
        //Set the scan register value
        for(int vfat=0; vfat<24; ++vfat){
            if ( !( (notmask >> vfat) & 0x1)) continue;
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfat,scanReg.c_str());
            writeReg(la, regBuf, dacVal);
        } //End Loop Over all VFATs

        //Reset the counters
        writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.TRIG.CNT.RESET",ohN), 0x1);

        //Wait just over 1 second
        std::this_thread::sleep_for(std::chrono::milliseconds(1005));

        //Read the counters
        int idx = (dacVal-dacMin)/dacStep;
        outDataDacVal[idx] = dacVal;
        outDataTrigRateOverall[idx] = readRawAddress(ohTrigRateAddr[24], la->response);
        for(int vfat=0; vfat<24; ++vfat){
            if ( !( (notmask >> vfat) & 0x1)) continue;

            idx = vfat*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
            outDataTrigRatePerVFAT[idx] = readRawAddress(ohTrigRateAddr[vfat], la->response);
        }
    } //End Loop from dacMin to dacMax

    //Restore the original channel masks if specific channel was requested
    if( ch != 128) {
        for(int vfat=0; vfat<24; ++vfat){
            if ( !( (notmask >> vfat) & 0x1)) continue;
            applyChanMask(map_chanOrigMask[vfat], la);
        }
    }
    return;
} //End sbitRateScanParallel(...)

/*! \fn void sbitRateScan(const RPCMsg *request, RPCMsg *response)
 *  \brief SBIT rate scan. See the local callable methods documentation for details 
 *  \param request RPC response message
 *  \param response RPC response message
 */
void sbitRateScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t maskOh = request->get_word("maskOh");
    bool invertVFATPos = request->get_word("invertVFATPos");
    uint32_t ch = request->get_word("ch");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");
    std::string scanReg = request->get_string("scanReg");
    uint32_t waitTime = request->get_word("waitTime");
    bool isParallel = request->get_word("isParallel");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outDataDacVal[(dacMax-dacMin+1)/dacStep];
    uint32_t outDataTrigRate[(dacMax-dacMin+1)/dacStep];
    if(isParallel){
        uint32_t outDataTrigRatePerVFAT[24*(dacMax-dacMin+1)/dacStep];
        sbitRateScanParallelLocal(&la, outDataDacVal, outDataTrigRatePerVFAT, outDataTrigRate, ohN, maskOh, ch, dacMin, dacMax, dacStep, scanReg);

        response->set_word_array("data_trigRatePerVFAT", outDataTrigRatePerVFAT, 24*(dacMax-dacMin+1)/dacStep);
    }
    else{
        sbitRateScanLocal(&la, outDataDacVal, outDataTrigRate, ohN, maskOh, invertVFATPos, ch, dacMin, dacMax, dacStep, scanReg, waitTime);
    }

    response->set_word_array("data_dacVal", outDataDacVal, (dacMax-dacMin+1)/dacStep);
    response->set_word_array("data_trigRate", outDataTrigRate, (dacMax-dacMin+1)/dacStep);

    return;
} //End sbitRateScan(...)

/*! \fn void genChannelScan(const RPCMsg *request, RPCMsg *response)
 *  \brief Generic per channel scan. See the local callable methods documentation for details 
 *  \param request RPC response message
 *  \param response RPC response message
 */
void genChannelScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t nevts = request->get_word("nevts");
    uint32_t ohN = request->get_word("ohN");
    uint32_t mask = request->get_word("mask");
    uint32_t dacMin = request->get_word("dacMin");
    uint32_t dacMax = request->get_word("dacMax");
    uint32_t dacStep = request->get_word("dacStep");
    bool useCalPulse = request->get_word("useCalPulse");
    bool currentPulse = request->get_word("currentPulse");
    uint32_t calScaleFactor = request->get_word("calScaleFactor");
    bool useExtTrig = request->get_word("useExtTrig");
    std::string scanReg = request->get_string("scanReg");

    bool useUltra = false;
    if (request->get_key_exists("useUltra")){
        useUltra = true;
    }

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[128*24*(dacMax-dacMin+1)/dacStep];
    for(uint32_t ch = 0; ch < 128; ch++)
    {
        genScanLocal(&la, &(outData[ch*24*(dacMax-dacMin+1)/dacStep]), ohN, mask, ch, useCalPulse, currentPulse, calScaleFactor, nevts, dacMin, dacMax, dacStep, scanReg, useUltra, useExtTrig);
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
