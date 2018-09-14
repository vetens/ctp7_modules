/*! \file vfat3.cpp
 *  \brief RPC module for VFAT3 methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#include <algorithm>
#include <chrono>
#include "optohybrid.h"
#include <thread>
#include "vfat3.h"

uint32_t vfatSyncCheckLocal(localArgs * la, uint32_t ohN)
{
    uint32_t goodVFATs = 0;
    for(int vfatN = 0; vfatN < 24; vfatN++)
    {
        char regBase [100];
        sprintf(regBase, "GEM_AMC.OH_LINKS.OH%i.VFAT%i",ohN, vfatN);
        bool linkGood = readReg(la, std::string(regBase)+".LINK_GOOD");
        uint32_t linkErrors = readReg(la, std::string(regBase)+".SYNC_ERR_CNT");
        goodVFATs = goodVFATs | ((linkGood && (linkErrors == 0)) << vfatN);
    }

    return goodVFATs;
}

void vfatSyncCheck(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t goodVFATs = vfatSyncCheckLocal(&la, ohN);

    response->set_word("goodVFATs", goodVFATs);

    return;
}

void configureVFAT3DacMonitorLocal(localArgs *la, uint32_t ohN, uint32_t mask, uint32_t dacSelect){
    //Check if VFATs are sync'd
    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    uint32_t notmask = ~mask & 0xFFFFFF;
    if( (notmask & goodVFATs) != notmask)
    {
        la->response->set_string("error",stdsprintf("One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask));
        return;
    }

    //Get ref voltage and monitor gain
    //These should have been set at time of configure
    uint32_t adcVRefValues[24];
    uint32_t monitorGainValues[24];
    broadcastReadLocal(la, adcVRefValues, ohN, "CFG_VREF_ADC", mask);
    broadcastReadLocal(la, monitorGainValues, ohN, "CFG_MON_GAIN", mask);

    //Loop over all vfats and set the dacSelect
    for(int vfatN=0; vfatN<24; ++vfatN){
        // Check if vfat is masked
        if(!((notmask >> vfatN) & 0x1)){
            continue;
        } //End check if VFAT is masked

        //Build global control 4 register
        uint32_t glbCtr4 = (adcVRefValues[vfatN] << 8) + (monitorGainValues[vfatN] << 7) + dacSelect;
        writeReg(la, stdsprintf("GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_4",ohN,vfatN), glbCtr4);
    } //End loop over all VFATs

    return;
} //End configureVFAT3DacMonitorLocal(...)

void configureVFAT3DacMonitor(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    uint32_t vfatMask = request->get_word("vfatMask");
    uint32_t dacSelect = request->get_word("dacSelect");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    LOGGER->log_message(LogManager::INFO, stdsprintf("Programming VFAT3 ADC Monitoring for Selection %i",dacSelect));
    configureVFAT3DacMonitorLocal(&la, ohN, vfatMask, dacSelect);

    return;
} //End configureVFAT3DacMonitor()

void configureVFAT3DacMonitorMultiLink(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohMask = request->get_word("ohMask");
    uint32_t ohVfatMaskArray[12];
    request->get_word_array("ohVfatMaskArray",ohVfatMaskArray);
    uint32_t dacSelect = request->get_word("dacSelect");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    for(int ohN=0; ohN<12; ++ohN){
        // If this Optohybrid is masked skip it
        if(!((ohMask >> ohN) & 0x1)){
            continue;
        }

        LOGGER->log_message(LogManager::INFO, stdsprintf("Programming VFAT3 ADC Monitoring on OH%i for Selection %i",ohN,dacSelect));
        configureVFAT3DacMonitorLocal(&la, ohN, ohVfatMaskArray[ohN], dacSelect);
    } //End Loop over all Optohybrids

    return;
} //End configureVFAT3DacMonitorMultiLink()

void configureVFAT3sLocal(localArgs * la, uint32_t ohN, uint32_t vfatMask) {
    std::string line, regName;
    uint32_t dacVal;
    std::string dacName;
    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    uint32_t notmask = ~vfatMask & 0xFFFFFF;
    if( (notmask & goodVFATs) != notmask)
    {
        char errBuf[200];
        sprintf(errBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        la->response->set_string("error",errBuf);
        return;
    }

    LOGGER->log_message(LogManager::INFO, "Load configuration settings");
    for(uint32_t vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
    {
        std::string configFileBase = "/mnt/persistent/gemdaq/vfat3/config_OH"+std::to_string(ohN)+"_VFAT"+std::to_string(vfatN)+".txt";
        std::ifstream infile(configFileBase);
        if(!infile.is_open())
        {
            LOGGER->log_message(LogManager::ERROR, "could not open config file "+configFileBase);
            la->response->set_string("error", "could not open config file "+configFileBase);
            return;
        }
        std::getline(infile,line);// skip first line
        while (std::getline(infile,line))
        {
            std::string reg_basename = "GEM_AMC.OH.OH" + std::to_string(ohN) + ".GEB.VFAT"+std::to_string(vfatN)+".CFG_";
            std::stringstream iss(line);
            if (!(iss >> dacName >> dacVal)) {
                LOGGER->log_message(LogManager::ERROR, "ERROR READING SETTINGS");
                la->response->set_string("error", "Error reading settings");
                break;
            }
            else
            {
                regName = reg_basename + dacName;
                writeReg(la, regName, dacVal);
            }
        }
    }
}

void configureVFAT3s(const RPCMsg *request, RPCMsg *response) {
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);
    uint32_t ohN = request->get_word("ohN");
    uint32_t vfatMask = request->get_word("vfatMask");
    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    configureVFAT3sLocal(&la, ohN, vfatMask);
    rtxn.abort();
}

void getChannelRegistersVFAT3(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);
    LOGGER->log_message(LogManager::INFO, "Getting VFAT3 Channel Registers");

    uint32_t ohN = request->get_word("ohN");
    uint32_t vfatMask = request->get_word("vfatMask");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t chanRegData[24*128];

    getChannelRegistersVFAT3Local(&la, ohN, vfatMask, chanRegData);

    response->set_word_array("chanRegData",chanRegData,24*128);

    return;
} //End getChannelRegistersVFAT3()

void getChannelRegistersVFAT3Local(localArgs *la, uint32_t ohN, uint32_t vfatMask, uint32_t *chanRegData){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~vfatMask & 0xFFFFFF;

    char regBuf[200];
    LOGGER->log_message(LogManager::INFO, "Read channel register settings");
    for(int vfatN=0; vfatN < 24; ++vfatN){
        // Check if vfat is masked
        if(!((notmask >> vfatN) & 0x1)){
            continue;
        } //End check if VFAT is masked

        // Check if vfatN is sync'd
        uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
        if( !( (goodVFATs >> vfatN ) & 0x1 ) ){
            sprintf(regBuf,"The requested VFAT is not synced; goodVFATs: %x\t requested VFAT: %i; maskOh: %x", goodVFATs, vfatN, vfatMask);
            la->response->set_string("error",regBuf);
            return;
        }

        //Loop over the channels
        uint32_t chanAddr;
        for(int chan=0; chan < 128; ++chan){
            //Deterime the idx
            int idx = vfatN*128 + chan;

            //Get the address
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i",ohN,vfatN,chan);
            chanAddr = getAddress(la, regBuf);

            //Build the channel register
            LOGGER->log_message(LogManager::INFO, stdsprintf("Reading channel register for VFAT%i chan %i",vfatN,chan));
            chanRegData[idx] = readRawAddress(chanAddr, la->response);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        } //End Loop over channels
    } //End Loop over VFATs

    return;
} //end getChannelRegistersVFAT3Local()

void readVFAT3ADCLocal(localArgs * la, uint32_t * outData, uint32_t ohN, bool useExtRefADC, uint32_t mask){
    if(useExtRefADC){ //Case: Use ADC with external reference
        broadcastReadLocal(la, outData, ohN, "ADC1", mask);
    } //End Case: Use ADC with external reference
    else{ //Case: Use ADC with internal reference
        broadcastReadLocal(la, outData, ohN, "ADC0", mask);
    } //End Case: Use ADC with internal reference

    return;
} //End readVFAT3ADCLocal

void readVFAT3ADC(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");
    bool useExtRefADC = request->get_word("useExtRefADC");
    uint32_t vfatMask = request->get_word("vfatMask");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t adcData[24];

    LOGGER->log_message(LogManager::INFO, stdsprintf("Reading VFAT3 ADC's for OH%i with mask %x",ohN, vfatMask));
    readVFAT3ADCLocal(&la, adcData, ohN, useExtRefADC, vfatMask);

    response->set_word_array("adcData",adcData,24);

    return;
} //End getChannelRegistersVFAT3()

void readVFAT3ADCMultiLink(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohMask = request->get_word("ohMask");
    uint32_t ohVfatMaskArray[12];
    request->get_word_array("ohVfatMaskArray",ohVfatMaskArray);
    bool useExtRefADC = request->get_word("useExtRefADC");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t adcData[24] = {0};
    uint32_t adcDataAll[12*24] = {0};
    for(int ohN=0; ohN<12; ++ohN){
        // If this Optohybrid is masked skip it
        if(!((ohMask >> ohN) & 0x1)){
            continue;
        }

        LOGGER->log_message(LogManager::INFO, stdsprintf("Reading VFAT3 ADC Values for all chips on OH%i",ohN));

        //Get all ADC values
        readVFAT3ADCLocal(&la, adcData, ohN, useExtRefADC, ohVfatMaskArray[ohN]);

        //Copy all ADC values
        std::copy(adcData, adcData+24, adcDataAll+(24*ohN));
    } //End Loop over all Optohybrids

    response->set_word_array("adcDataAll",adcDataAll,12*24);

    return;
} //End readVFAT3ADCMultiLink()

void setChannelRegistersVFAT3SimpleLocal(localArgs * la, uint32_t ohN, uint32_t vfatMask, uint32_t *chanRegData){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~vfatMask & 0xFFFFFF;

    char regBuf[200];
    LOGGER->log_message(LogManager::INFO, "Write channel register settings");
    for(int vfatN=0; vfatN < 24; ++vfatN){
        // Check if vfat is masked
        if(!((notmask >> vfatN) & 0x1)){
            continue;
        } //End check if VFAT is masked

        // Check if vfatN is sync'd
        uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
        if( !( (goodVFATs >> vfatN ) & 0x1 ) ){
            sprintf(regBuf,"The requested VFAT is not synced; goodVFATs: %x\t requested VFAT: %i; maskOh: %x", goodVFATs, vfatN, vfatMask);
            la->response->set_string("error",regBuf);
            return;
        }

        //Loop over the channels
        uint32_t chanAddr;
        for(int chan=0; chan < 128; ++chan){
            //Deterime the idx
            int idx = vfatN*128 + chan;

            //Get the address
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i",ohN,vfatN,chan);
            chanAddr = getAddress(la, regBuf);
            writeRawAddress(chanAddr, chanRegData[idx], la->response);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        } //End Loop over channels
    } //End Loop over VFATs

    return;
} //End setChannelRegistersVFAT3SimpleLocal()

void setChannelRegistersVFAT3Local(localArgs * la, uint32_t ohN, uint32_t vfatMask, uint32_t *calEnable, uint32_t *masks, uint32_t *trimARM, uint32_t *trimARMPol, uint32_t *trimZCC, uint32_t *trimZCCPol){
    //Determine the inverse of the vfatmask
    uint32_t notmask = ~vfatMask & 0xFFFFFF;

    char regBuf[200];
    LOGGER->log_message(LogManager::INFO, "Write channel register settings");
    for(int vfatN=0; vfatN < 24; ++vfatN){
        // Check if vfat is masked
        if(!((notmask >> vfatN) & 0x1)){
            continue;
        } //End check if VFAT is masked

        // Check if vfatN is sync'd
        uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
        if( !( (goodVFATs >> vfatN ) & 0x1 ) ){
            sprintf(regBuf,"The requested VFAT is not synced; goodVFATs: %x\t requested VFAT: %i; maskOh: %x", goodVFATs, vfatN, vfatMask);
            la->response->set_string("error",regBuf);
            return;
        }

        //Loop over the channels
        uint32_t chanAddr;
        uint32_t chanRegVal;
        for(int chan=0; chan < 128; ++chan){
            //Deterime the idx
            int idx = vfatN*128 + chan;

            //Get the address
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i",ohN,vfatN,chan);
            chanAddr = getAddress(la, regBuf);

            //Check trim values make sense
            if ( trimARM[idx] > 0x3F || trimARM[idx] < 0x0){
                sprintf(regBuf,"arming comparator trim value must be positive in range [0x0,0x3F]. Value given for VFAT%i chan %i: %x",vfatN,chan,trimARM[idx]);
                la->response->set_string("error",regBuf);
                return;
            }
            if ( trimZCC[idx] > 0x3F || trimZCC[idx] < 0x0){
                sprintf(regBuf,"zero crossing comparator trim value must be positive in range [0x0,0x3F]. Value given for VFAT%i chan %i: %x",vfatN,chan,trimZCC[idx]);
                la->response->set_string("error",regBuf);
                return;
            }

            //Build the channel register
            LOGGER->log_message(LogManager::INFO, stdsprintf("Setting channel register for VFAT%i chan %i",vfatN,chan));
            chanRegVal = (calEnable[idx] << 15) + (masks[idx] << 14) + \
                         (trimZCCPol[idx] << 13) + (trimZCC[idx] << 7) + \
                         (trimARMPol[idx] << 6) + (trimARM[idx]);
            writeRawAddress(chanAddr, chanRegVal, la->response);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        } //End Loop over channels
    } //End Loop over VFATs

    return;
} //end setChannelRegistersVFAT3Local()

void setChannelRegistersVFAT3(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);
    LOGGER->log_message(LogManager::INFO, "Setting VFAT3 Channel Registers");

    uint32_t ohN = request->get_word("ohN");
    uint32_t vfatMask = request->get_word("vfatMask");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    if (request->get_key_exists("simple")){
        uint32_t chanRegData[3072];

        request->get_word_array("chanRegData",chanRegData);

        setChannelRegistersVFAT3SimpleLocal(&la, ohN, vfatMask, chanRegData);
    } //End Case: user provided a single array
    else{ //Case: user provided multiple arrays
        uint32_t calEnable[3072];
        uint32_t masks[3072];
        uint32_t trimARM[3072];
        uint32_t trimARMPol[3072];
        uint32_t trimZCC[3072];
        uint32_t trimZCCPol[3072];

        request->get_word_array("calEnable",calEnable);
        request->get_word_array("masks",masks);
        request->get_word_array("trimARM",trimARM);
        request->get_word_array("trimARMPol",trimARMPol);
        request->get_word_array("trimZCC",trimZCC);
        request->get_word_array("trimZCCPol",trimZCCPol);

        setChannelRegistersVFAT3Local(&la, ohN, vfatMask, calEnable, masks, trimARM, trimARMPol, trimZCC, trimZCCPol);
    } //End Case: user provided multiple arrays

    return;
} //End setChannelRegistersVFAT3()

void statusVFAT3sLocal(localArgs * la, uint32_t ohN)
{
    std::string regs [] = {"CFG_PULSE_STRETCH ",
                           "CFG_SYNC_LEVEL_MODE",
                           "CFG_FP_FE",
                           "CFG_RES_PRE",
                           "CFG_CAP_PRE",
                           "CFG_PT",
                           "CFG_SEL_POL",
                           "CFG_FORCE_EN_ZCC",
                           "CFG_SEL_COMP_MODE",
                           "CFG_VREF_ADC",
                           "CFG_IREF",
                           "CFG_THR_ARM_DAC",
                           "CFG_LATENCY",
                           "CFG_CAL_SEL_POL",
                           "CFG_CAL_DAC",
                           "CFG_CAL_MODE",
                           "CFG_BIAS_CFD_DAC_2",
                           "CFG_BIAS_CFD_DAC_1",
                           "CFG_BIAS_PRE_I_BSF",
                           "CFG_BIAS_PRE_I_BIT",
                           "CFG_BIAS_PRE_I_BLCC",
                           "CFG_BIAS_PRE_VREF",
                           "CFG_BIAS_SH_I_BFCAS",
                           "CFG_BIAS_SH_I_BDIFF",
                           "CFG_BIAS_SH_I_BFAMP",
                           "CFG_BIAS_SD_I_BDIFF",
                           "CFG_BIAS_SD_I_BSF",
                           "CFG_BIAS_SD_I_BFCAS",
                           "CFG_RUN"};
    std::string regName;

    for(int vfatN = 0; vfatN < 24; vfatN++)
    {
        char regBase [100];
        sprintf(regBase, "GEM_AMC.OH_LINKS.OH%i.VFAT%i.",ohN, vfatN);
        for (auto &reg : regs) {
            regName = std::string(regBase)+reg;
            la->response->set_word(regName,readReg(la,regName));
        }
    }
}

void statusVFAT3s(const RPCMsg *request, RPCMsg *response) {
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);
    uint32_t ohN = request->get_word("ohN");
    LOGGER->log_message(LogManager::INFO, "Reading VFAT3 status");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    statusVFAT3sLocal(&la, ohN);
    rtxn.abort();
}

extern "C" {
    const char *module_version_key = "vfat3 v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memsvc_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("vfat3", "configureVFAT3s", configureVFAT3s);
        modmgr->register_method("vfat3", "configureVFAT3DacMonitor", configureVFAT3DacMonitor);
        modmgr->register_method("vfat3", "configureVFAT3DacMonitorMultiLink", configureVFAT3DacMonitorMultiLink);
        modmgr->register_method("vfat3", "getChannelRegistersVFAT3", getChannelRegistersVFAT3);
        modmgr->register_method("vfat3", "readVFAT3ADC", readVFAT3ADC);
        modmgr->register_method("vfat3", "readVFAT3ADCMultiLink", readVFAT3ADCMultiLink);
        modmgr->register_method("vfat3", "setChannelRegistersVFAT3", setChannelRegistersVFAT3);
        modmgr->register_method("vfat3", "statusVFAT3s", statusVFAT3s);
        modmgr->register_method("vfat3", "vfatSyncCheck", vfatSyncCheck);
    }
}
