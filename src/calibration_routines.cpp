/*! \file calibration_routines.cpp
 *  \brief Calibration routines
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#include <math.h>
#include <chrono>
#include <pthread.h>
#include "optohybrid.h"
#include <thread>
#include "vfat3.h"
#include "utils.h"

/*! \fn unsigned int fw_version_check(const char* caller_name, localArgs *la)
 *  \brief Returns AMC FW version
 *  in case FW version is not 1.X or 3.X sets an error string in response
 *  \param caller_name Name of methods which called the FW version check
 *  \param la Local arguments structure
 */
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
        map_chanOrigMask[chanMaskAddr]=readReg(la, regBuf);

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

/*! \fn void confCalPulseLocal(localArgs *la, uint32_t ohN, uint32_t mask, uint32_t ch, bool toggleOn, bool currentPulse, uint32_t calScaleFactor)
 *  \brief Configures the calibration pulse for channel ch on all VFATs of ohN that are not in mask to either be on (toggleOn==true) or off (toggleOn==false).  If ch == 128 and toggleOn == False will write the CALPULSE_ENABLE bit for all channels of all vfats that are not masked on ohN to 0x0.
 *  \param la Local arguments structure
 *  \param ohN Optical link number
 *  \param mask VFAT mask
 *  \param ch Channel of interest
 *  \param toggleOn if true (false) turns the calibration pulse on (off) for channel ch
 *  \param currentPulse Selects whether to use current or volage pulse
 *  \param calScaleFactor Scale factor for the calibration pulse height (00 = 25%, 01 = 50%, 10 = 75%, 11 = 100%)
 */
bool confCalPulseLocal(localArgs *la, uint32_t ohN, uint32_t mask, uint32_t ch, bool toggleOn, bool currentPulse, uint32_t calScaleFactor){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    char regBuf[200];
    if(ch >= 128 && toggleOn == true){ //Case: Bad Config, asked for OR of all channels
        la->response->set_string("error","confCalPulseLocal(): I was told to calpulse all channels which doesn't make sense");
        return false;
    } //End Case: Bad Config, asked for OR of all channels
    else if(ch == 128 && toggleOn == false){ //Case: Turn cal pusle off for all channels
        for(int vfatN = 0; vfatN < 24; vfatN++){ //Loop over all VFATs
            if((notmask >> vfatN) & 0x1){ //End VFAT is not masked
                for(int chan=0; chan < 128; ++chan){ //Loop Over all Channels
                    sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, chan);
                    writeReg(la, regBuf, 0x0);
                } //End Loop Over all Channels
                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x0);
            } //End VFAT is not masked
        } //End Loop over all VFATs
    } //End Case: Turn cal pulse off for all channels
    else{ //Case: Pulse a specific channel
        for(int vfatN = 0; vfatN < 24; vfatN++){ //Loop over all VFATs
            if((notmask >> vfatN) & 0x1){ //End VFAT is not masked
                sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
                if(toggleOn == true){ //Case: turn calpulse on
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
                } //End Case: Turn calpulse on
                else{ //Case: Turn calpulse off
                    writeReg(la, regBuf, 0x0);
                    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_CAL_MODE", ohN, vfatN), 0x0);
                } //End Case: Turn calpulse off
            } //End VFAT is not masked
        } //End Loop over all VFATs
    } //End Case: Pulse a specific channel

    return true;
} //End confCalPulseLocal

/*! \fn void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch)
 *  \brief Configures DAQ monitor. Local version only
 *  \param la Local arguments structure
 *  \param ohN Optical link number
 *  \param ch Channel of interest
 */
void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch)
{
    //Check the firmware version
    char regBuf[200];
    switch (fw_version_check("dacMonConf", la)){
        case 3:
        {
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
            break;
        }// end v3 electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "dacMonConf is only supported in V3 electronics");
            sprintf(regBuf,"dacMonConf is only supported in V3 electronics");
            la->response->set_string("error",regBuf);
            break;
        }
    }
    return;
}

/*! \fn void ttcGenToggleLocal(localArgs * la, uint32_t ohN, bool enable)
 *  \brief Toggles the TTC Generator. Local callable version of ttcGenToggle
 *
 *  * v3  electronics: enable = true (false) turn on CTP7 internal TTC generator and ignore ttc commands from backplane for this AMC (turn off CTP7 internal TTC generator and take ttc commands from backplane link)
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
                writeReg(la, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x1); //Internal TTC generator enabled, TTC cmds from backplane are ignored
            }
            else{
                writeReg(la, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x0); //Internal TTC generator disabled, TTC cmds from backplane
            }
            break;
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
            break;
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major, do nothing");
            break;
        }
    }
    return;
} //End ttcGenToggleLocal(...)

/*! \fn void ttcGenToggle(const RPCMsg *request, RPCMsg *response)
 *  \brief Toggles the TTC Generator
 *
 *  * v3  electronics: enable = true (false) turn on CTP7 internal TTC generator and ignore ttc commands from backplane for this AMC (turn off CTP7 internal TTC generator and take ttc commands from backplane link)
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
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
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
 *  - **v3**  electronics behavior:
 *    * pulseDelay (only for enable = true), delay between CalPulse and L1A
 *    * L1Ainterval (only for enable = true), how often to repeat signals
 *    * enable = true (false) turn on CTP7 internal TTC generator and ignore ttc commands from backplane for this AMC (turn off CTP7 internal TTC generator and take ttc commands from backplane link)
 *  - **v2b** electronics behavior:
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
    switch(fw_version_check("ttcGenConf", la)) {
        case 0x3: //v3 electronics behavior
        {
            writeReg(la, "GEM_AMC.TTC.GENERATOR.RESET", 0x1);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_GAP", L1Ainterval);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_CALPULSE_TO_L1A_GAP", pulseDelay);
            break;
        }//End v3 electronics behavior
        case 0x1: //v2b electronics behavior
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
            break;
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major, do nothing");
            break;
        }
    }
    //start or stop
    ttcGenToggleLocal(la, ohN, enable);
    return;
}

/*! \fn void ttcGenConf(const RPCMsg *request, RPCMsg *response)
 *  \brief Configures TTC generator
 *
 *  - **v3**  electronics behavior:
 *    * pulseDelay (only for enable = true), delay between CalPulse and L1A
 *    * L1Ainterval (only for enable = true), how often to repeat signals
 *    * enable = true (false) ignore (take) ttc commands from backplane for this AMC (affects all links)
 *  - **v2b** electronics behavior:
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
 *  \param request RPC request message
 *  \param response RPC response message
 */
void ttcGenConf(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
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
    switch(fw_version_check("genScanLocal", la)) {
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
                if (confCalPulseLocal(la, ohN, mask, ch, true, currentPulse, calScaleFactor) == false){
                    return; //Calibration pulse is not configured correctly
                }
            } //End use calibration pulse

            //Get addresses
            uint32_t daqMonAddr[24];
            uint32_t l1CntAddr = getAddress(la, "GEM_AMC.TTC.CMD_COUNTERS.L1A");
            for(int vfatN = 0; vfatN < 24; vfatN++)
            {
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

                //Stop the DAQ monitor counters from incrementing
                writeReg(la, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.ENABLE", 0x0);

                //Read the DAQ Monitor counters
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
                if (confCalPulseLocal(la, ohN, mask, ch, false, currentPulse, calScaleFactor) == false){
                    return; //Calibration pulse is not configured correctly
                }
            }
            break;
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
            break;
        }//End v2b electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "Unexpected value for system release major, do nothing");
            break;
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
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
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
 *  \waitTime Measurement duration per point in milliseconds
 */
void sbitRateScanLocal(localArgs *la, uint32_t *outDataDacVal, uint32_t *outDataTrigRate, uint32_t ohN, uint32_t maskOh, bool invertVFATPos, uint32_t ch, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg, uint32_t waitTime)
{
    char regBuf[200];
    switch (fw_version_check("SBIT Rate Scan", la)){
        case 3:
        {
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
            sprintf(regBuf,"GEM_AMC.OH.OH%i.FPGA.TRIG.CTRL.VFAT_MASK",ohN);
            uint32_t ohVFATMaskAddr = getAddress(la, regBuf);
            uint32_t maskOhOrig = readRawAddress(ohVFATMaskAddr, la->response);   //We'll write this later
            writeRawAddress(ohVFATMaskAddr, maskOh, la->response);

            //Take the VFATs out of slow control only mode
            writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

            //Loop from dacMin to dacMax in steps of dacStep
            for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
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

            break;
        }//End v3 electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "sbitRateScan is only supported in V3 electronics");
            sprintf(regBuf,"sbitRateScan is only supported in V3 electronics");
            la->response->set_string("error",regBuf);
            break;
        }
    }
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
    switch (fw_version_check("SBIT Rate Scan", la)){
        case 3:
        {
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
                sprintf(regBuf,"GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.VFAT%i_SBITS",ohN,vfat);
                ohTrigRateAddr[vfat] = getAddress(la, regBuf);
            } //End Loop over all VFATs

            //Take the VFATs out of slow control only mode
            writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

            //Prep the SBIT counters
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.SBIT_CNT_PERSIST",ohN), 0x0); //reset all counters after SBIT_CNT_TIME_MAX
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.SBIT_CNT_TIME_MAX",ohN), 0x02638e98); //count for 1 second

            //Loop from dacMin to dacMax in steps of dacStep
            for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep){
                //Set the scan register value
                for(int vfat=0; vfat<24; ++vfat){
                    if ( !( (notmask >> vfat) & 0x1)) continue;
                    sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfat,scanReg.c_str());
                    writeReg(la, regBuf, dacVal);
                } //End Loop Over all VFATs

                //Reset the counters
                writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.RESET",ohN), 0x1);

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
            break;
        }//End v3 electronics behavior
        default:
        {
            LOGGER->log_message(LogManager::ERROR, "sbitRateScan is only supported in V3 electronics");
            sprintf(regBuf,"sbitRateScan is only supported in V3 electronics");
            la->response->set_string("error",regBuf);
            break;
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
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
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

/*! \fn void checkSbitMappingWithCalPulseLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, bool currentPulse, uint32_t calScaleFactor, uint32_t nevts, uint32_t L1Ainterval, uint32_t pulseDelay)
 *  \brief With all but one channel masked, pulses a given channel, and then checks which sbits are seen by the CTP7, repeats for all channels; reports the (vfat,chan) pulsed and (vfat,sbit) observed where sbit=chan*2; additionally reports if the cluster was valid
 *  \param la Local arguments structure
 *  \param outData pointer to an array of size (24*128*8*nevts) which stores the results of the scan, bits [0,7] channel pulsed; bits [8:15] sbit observed; bits [16:20] vfat pulsed; bits [21,25] vfat observed; bit 26 isValid; bits [27,29] are the cluster size
 *  \param ohN Optical link
 *  \param mask VFAT mask
 *  \param currentPulse Selects whether to use current or volage pulse
 *  \param calScaleFactor
 *  \param nevts the number of cal pulses to inject per channel
 *  \param L1Ainterval How often to repeat signals (only for enable = true)
 *  \param pulseDelay delay between CalPulse and L1A
 */
void checkSbitMappingWithCalPulseLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, bool currentPulse, uint32_t calScaleFactor, uint32_t nevts, uint32_t L1Ainterval, uint32_t pulseDelay){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    char regBuf[200];
    if( fw_version_check("checkSbitMappingWithCalPulse", la) < 3){
        LOGGER->log_message(LogManager::ERROR, "checkSbitMappingWithCalPulse is only supported in V3 electronics");
        sprintf(regBuf,"checkSbitMappingWithCalPulse is only supported in V3 electronics");
        la->response->set_string("error",regBuf);
        return;
    }

    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    if( (notmask & goodVFATs) != notmask){
        sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        la->response->set_string("error",regBuf);
        return;
    }

    if (currentPulse && calScaleFactor > 3){
        sprintf(regBuf,"Bad value for CFG_CAL_FS: %x, Possible values are {0b00, 0b01, 0b10, 0b11}. Exiting.",calScaleFactor);
        la->response->set_string("error",regBuf);
        return;
    }

    //Get current channel register data, mask all channels and disable calpulse
    uint32_t chanRegData_orig[3072]; //original channel register data
    uint32_t chanRegData_tmp[3072]; //temporary channel register data
    getChannelRegistersVFAT3Local(la, ohN, mask, chanRegData_orig);
    for(int idx=0; idx < 3072; ++idx){
        chanRegData_tmp[idx]=chanRegData_orig[idx]+(0x1 << 14); //set channel mask to true
        chanRegData_tmp[idx]=chanRegData_tmp[idx]-(0x1 << 15); //disable calpulse
    }
    setChannelRegistersVFAT3SimpleLocal(la, ohN, mask, chanRegData_tmp);

    //Setup TTC Generator
    ttcGenConfLocal(la, ohN, 0, 0, pulseDelay, L1Ainterval, nevts, true);
    writeReg(la, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1);
    writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", 0x1); //One pulse at a time
    uint32_t addrTtcStart = getAddress(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_START");

    //Set all chips out of run mode
    broadcastWriteLocal(la, ohN, "CFG_RUN", 0x0, mask);

    //Take the VFATs out of slow control only mode
    writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

    //Setup the sbit monitor
    writeReg(la, "GEM_AMC.TRIGGER.SBIT_MONITOR.OH_SELECT", ohN);
    uint32_t addrSbitMonReset=getAddress(la, "GEM_AMC.TRIGGER.SBIT_MONITOR.RESET");
    uint32_t addrSbitCluster[8];
    for(int iCluster=0; iCluster < 8; ++iCluster){
        sprintf(regBuf, "GEM_AMC.TRIGGER.SBIT_MONITOR.CLUSTER%i",iCluster);
        addrSbitCluster[iCluster] = getAddress(la, regBuf);
    }

    int posPerEvt=8; //8 clusters will be readout per injected calpulse
    int posPerChan=nevts*posPerEvt;
    int posPerVFAT=128*posPerChan;

    //Monitor which sbit is seen when sending a calupluse
    for(int vfatN=0; vfatN < 24; ++vfatN){ //Loop over all vfats
        //Skip masked vfats
        if((notmask >> vfatN) & 0x0){
            continue;
        }

        //mask all other vfats from trigger
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK",ohN), 0xffffff & ~(1 << (vfatN)));

        //Place this vfat into run mode
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN, vfatN), 0x1);

        for(int chan=0; chan < 128; ++chan){ //Loop over all channels
            //unmask this channel
            //May want to move this inside the pulse block?
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,vfatN,chan), 0x0);

            //Turn on the calpulse for this channel
            if (confCalPulseLocal(la, ohN, ((0x1)<<vfatN), chan, false, currentPulse, calScaleFactor) == false){
                return; //Calibration pulse is not configured correctly
            }

            //Start Pulsing
            for(unsigned int iPulse=0; iPulse < nevts; ++iPulse){ //Pulse this channel
                //Reset monitors
                writeRawAddress(addrSbitMonReset, 0x1, la->response);

                //Start the TTC Generator
                writeRawAddress(addrTtcStart, 0x1, la->response);

                //Sleep for 200 us + pulseDelay * 25 ns * (0.001 us / ns)
                std::this_thread::sleep_for(std::chrono::microseconds(200+int(ceil(pulseDelay*25*0.001))));

                //Check clusers
                for(int cluster=0; cluster<8; ++cluster){
                    int idx = vfatN * posPerVFAT + chan * posPerChan + iPulse * posPerEvt + cluster; //Array index

                    //bits [10:0] is the address of the cluster
                    //bits [14:12] is the cluster size
                    //bits 15 and 11 are not used
                    uint32_t thisCluster = readRawAddress(addrSbitCluster[cluster], la->response);
                    uint32_t address = (thisCluster & 0x7ff);
                    uint32_t isValid = (address < 0x7fa); //If there's no sbit this should point to an invalid address, 0x7fa
                    if (isValid){ //Case: Valid Cluster
                        uint32_t clusterSize = (thisCluster & 0x7000);
                        uint32_t hwVfatPos = int(address / 64); //Note: hwVfatPos != vfatN
                        uint32_t partition = int(hwVfatPos / 3);
                        uint32_t column = hwVfatPos % 3;

                        outData[idx] = (clusterSize << 27) + (0x1 << 26) + ((column * 8 + (7-partition)) << 21) + (vfatN << 16) + ((address % 64) << 8) + chan;
                    } //End Case: Valid Cluster
                    else{ //Case: Cluster is not valid
                        //use observed vfatN = 31 and observed sbit = 255 for not valid case (e.g. overflow for those bits)
                        outData[idx] = (0x0 << 27) + (0x0 << 26) + (31 << 21) + (vfatN << 16) + (255 << 8) + chan;
                    } //End Case: Cluster is not valid
                } //End Loop over clusters
            } //End Pulses for this channel

            //mask this channel
            //may want to move this inside the pulse block?
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,vfatN,chan), 0x1);
        } //End Loop over all channels

        //Place this vfat out of run mode
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN, vfatN), 0x0);
    } //End Loop over all VFATs

    //turn off TTC Generator
    ttcGenToggleLocal(la, ohN, false);

    //Return channel register settings to their original values
    setChannelRegistersVFAT3SimpleLocal(la, ohN, mask, chanRegData_orig);

    //Set trigger vfat mask for this OH back to 0
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK",ohN), 0x0);

    return;
} //End checkSbitMappingWithCalPulseLocal(...)

/*! \fn void checkSbitMappingWithCalPulse(const RPCMsg *request, RPCMsg *response)
 *  \brief Checks the sbit mapping using the calibration pulse. See the local callable methods documentation for details
 *  \param request RPC response message
 *  \param response RPC response message
 */
void checkSbitMappingWithCalPulse(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t mask = request->get_word("mask");
    bool currentPulse = request->get_word("currentPulse");
    uint32_t calScaleFactor = request->get_word("calScaleFactor");
    uint32_t nevts = request->get_word("nevts");
    uint32_t L1Ainterval = request->get_word("L1Ainterval");
    uint32_t pulseDelay = request->get_word("pulseDelay");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*128*8*nevts];
    checkSbitMappingWithCalPulseLocal(&la, outData, ohN, mask, currentPulse, calScaleFactor, nevts, L1Ainterval, pulseDelay);

    response->set_word_array("data",outData,24*128*8*nevts);

    return;
} //End checkSbitMappingWithCalPulse()

/*! \fn void checkSbitRateWithCalPulseLocal(localArgs *la, uint32_t *outDataCTP7Rate, uint32_t *outDataFPGAClusterCntRate, uint32_t *outDataVFATSBits, uint32_t ohN, uint32_t mask, bool currentPulse, uint32_t calScaleFactor, uint32_t waitTime, uint32_t pulseRate, uint32_t pulseDelay)
 * \brief With all but one channel masked, pulses a given channel, and then checks the rate of sbits seen by the OH FPGA and CTP7, repeats for all channels; reports the rate observed
 *
 *  \param la Local arguments structure
 *  \param outDataCTP7Rate pointer to an array storing the value of GEM_AMC.TRIGGER.OHX.TRIGGER_RATE for X = ohN; array size 3072 elements, idx = 128 * vfat + chan
 *  \param outDataFPGAClusterCntRate as outDataCTP7Rate but for the value of GEM_AMC.OH.OHX.FPGA.TRIG.CNT.CLUSTER_COUNT
 *  \param outDataVFATSBits as outDataCTP7Rate but for the value of GEM_AMC.OH.OHX.FPGA.TRIG.CNT.VFATY_SBITS for X = ohN and Y the vfat number (following the array idx rule above)
 *  \param ohN Optical link
 *  \param mask VFAT mask
 *  \param currentPulse Selects whether to use current or volage pulse
 *  \param calScaleFactor
 *  \waitTime Measurement duration per point in milliseconds
 *  \param pulseRate rate of calpulses to be sent in Hz
 *  \param pulseDelay delay between CalPulse and L1A
 *
 */
void checkSbitRateWithCalPulseLocal(localArgs *la, uint32_t *outDataCTP7Rate, uint32_t *outDataFPGAClusterCntRate, uint32_t *outDataVFATSBits, uint32_t ohN, uint32_t mask, bool currentPulse, uint32_t calScaleFactor, uint32_t waitTime, uint32_t pulseRate, uint32_t pulseDelay){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~mask & 0xFFFFFF;

    char regBuf[200];
    if( fw_version_check("checkSbitRateWithCalPulse", la) < 3){
        LOGGER->log_message(LogManager::ERROR, "checkSbitRateWithCalPulse is only supported in V3 electronics");
        sprintf(regBuf,"checkSbitRateWithCalPulse is only supported in V3 electronics");
        la->response->set_string("error",regBuf);
        return;
    }

    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    if( (notmask & goodVFATs) != notmask){
        sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        la->response->set_string("error",regBuf);
        return;
    }

    if (currentPulse && calScaleFactor > 3){
        sprintf(regBuf,"Bad value for CFG_CAL_FS: %x, Possible values are {0b00, 0b01, 0b10, 0b11}. Exiting.",calScaleFactor);
        la->response->set_string("error",regBuf);
        return;
    }

    //Get current channel register data, mask all channels and disable calpulse
    uint32_t chanRegData_orig[3072]; //original channel register data
    uint32_t chanRegData_tmp[3072]; //temporary channel register data
    getChannelRegistersVFAT3Local(la, ohN, mask, chanRegData_orig);
    for(int idx=0; idx < 3072; ++idx){
        chanRegData_tmp[idx]=chanRegData_orig[idx]+(0x1 << 14); //set channel mask to true
        chanRegData_tmp[idx]=chanRegData_tmp[idx]-(0x1 << 15); //disable calpulse
    }
    setChannelRegistersVFAT3SimpleLocal(la, ohN, mask, chanRegData_tmp);

    //Setup TTC Generator
    uint32_t L1Ainterval = int(40079000 / pulseRate);
    uint32_t addrTtcReset = getAddress(la, "GEM_AMC.TTC.GENERATOR.RESET");
    uint32_t addrTtcStart = getAddress(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_START");

    //Get Trigger addresses
    uint32_t ohTrigRateAddr[26]; //idx 0->23 VFAT counters; idx 24 rate measured by OH FPGA; idx 25 rate measured by CTP7
    for(int vfat=0; vfat<24; ++vfat){
        ohTrigRateAddr[vfat] = getAddress(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.VFAT%i_SBITS",ohN,vfat));
    } //End Loop over all VFATs
    ohTrigRateAddr[24] = getAddress(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.CLUSTER_COUNT",ohN));
    ohTrigRateAddr[25] = getAddress(la, stdsprintf("GEM_AMC.TRIGGER.OH%i.TRIGGER_RATE",ohN));
    uint32_t addTrgCntResetOH = getAddress(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.RESET",ohN));
    uint32_t addTrgCntResetCTP7 = getAddress(la,"GEM_AMC.TRIGGER.CTRL.CNT_RESET");

    //Set all chips out of run mode
    broadcastWriteLocal(la, ohN, "CFG_RUN", 0x0, mask);

    //Take the VFATs out of slow control only mode
    writeReg(la, "GEM_AMC.GEM_SYSTEM.VFAT3.SC_ONLY_MODE", 0x0);

    //Prep the SBIT counters
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.SBIT_CNT_PERSIST",ohN), 0x0); //reset all counters after SBIT_CNT_TIME_MAX
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.FPGA.TRIG.CNT.SBIT_CNT_TIME_MAX",ohN), 0x02638e98); //count for 1 second

    for(int vfatN=0; vfatN < 24; ++vfatN){
        //Skip masked vfats
        if((notmask >> vfatN) & 0x0){
            continue;
        }

        //mask all other vfats from trigger
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK",ohN), 0xffffff & ~(1 << (vfatN)));

        //Place this vfat into run mode
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN, vfatN), 0x1);

        for(int chan=0; chan < 128; ++chan){ //Loop over all channels
            //unmask this channel
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,vfatN,chan), 0x0);

            //Turn on the calpulse for this channel
            if (confCalPulseLocal(la, ohN, ((0x1)<<vfatN), chan, false, currentPulse, calScaleFactor) == false){
                return; //Calibration pulse is not configured correctly
            }

            //Reset counters
            writeRawAddress(addTrgCntResetOH, 0x1, la->response);
            writeRawAddress(addTrgCntResetCTP7, 0x1, la->response);
            //also use daq monitor here...?

            //Start the TTC Generator
            ttcGenConfLocal(la, ohN, 0, 0, pulseDelay, L1Ainterval, 0, true);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1);
            writeReg(la, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", 0x0); //Continue until stopped
            writeRawAddress(addrTtcStart, 0x1, la->response);

            //Sleep for waitTime of milliseconds
            std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));

            //Read All Trigger Registers
            uint32_t idx = 128*vfatN + chan;
            outDataCTP7Rate[idx]=readRawAddress(ohTrigRateAddr[25], la->response);
            outDataFPGAClusterCntRate[idx]=readRawAddress(ohTrigRateAddr[24], la->response);
            outDataVFATSBits[idx]=readRawAddress(ohTrigRateAddr[vfatN], la->response);

            //Reset the TTC Generator
            writeRawAddress(addrTtcReset, 0x1, la->response);

            //mask this channel
            writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.MASK",ohN,vfatN,chan), 0x1);
        } //End Loop over all channels

        //Place this vfat out of run mode
        writeReg(la,stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN, vfatN), 0x0);
    } //End Loop over all vfats

    //turn off TTC Generator
    ttcGenToggleLocal(la, ohN, false);

    //Return channel register settings to their original values
    setChannelRegistersVFAT3SimpleLocal(la, ohN, mask, chanRegData_orig);

    //Set trigger vfat mask for this OH back to 0
    writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.TRIG.CTRL.VFAT_MASK",ohN), 0x0);

    return;
} //End checkSbitRateWithCalPulseLocal()

/*! \fn void checkSbitRateWithCalPulse(const RPCMsg *request, RPCMsg *response)
 *  \brief Checks the sbit rate using the calibration pulse. See the local callable methods documentation for details
 *  \param request RPC response message
 *  \param response RPC response message
 */
void checkSbitRateWithCalPulse(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t mask = request->get_word("mask");
    bool currentPulse = request->get_word("currentPulse");
    uint32_t calScaleFactor = request->get_word("calScaleFactor");
    uint32_t waitTime = request->get_word("waitTime");
    uint32_t pulseRate = request->get_word("pulseRate");
    uint32_t pulseDelay = request->get_word("pulseDelay");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outDataCTP7Rate[24*128];
    uint32_t outDataFPGAClusterCntRate[24*128];
    uint32_t outDataVFATSBits[24*128];
    checkSbitRateWithCalPulseLocal(&la, outDataCTP7Rate, outDataFPGAClusterCntRate, outDataVFATSBits, ohN, mask, currentPulse, calScaleFactor, waitTime, pulseRate, pulseDelay);

    response->set_word_array("outDataCTP7Rate",outDataCTP7Rate,24*128);
    response->set_word_array("outDataFPGAClusterCntRate",outDataFPGAClusterCntRate,24*128);
    response->set_word_array("outDataVFATSBits",outDataVFATSBits,24*128);

    return;
} //End checkSbitRateWithCalPulse()

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
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
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
        modmgr->register_method("calibration_routines", "checkSbitMappingWithCalPulse", checkSbitMappingWithCalPulse);
        modmgr->register_method("calibration_routines", "checkSbitRateWithCalPulse", checkSbitMappingWithCalPulse);
        modmgr->register_method("calibration_routines", "genScan", genScan);
        modmgr->register_method("calibration_routines", "genChannelScan", genChannelScan);
        modmgr->register_method("calibration_routines", "sbitRateScan", sbitRateScan);
        modmgr->register_method("calibration_routines", "ttcGenConf", ttcGenConf);
        modmgr->register_method("calibration_routines", "ttcGenToggle", ttcGenToggle);
    }
}
