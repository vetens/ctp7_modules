#include <pthread.h>
#include "optohybrid.h"
#include "vfat3.h"

void dacMonConfLocal(localArgs * la, uint32_t ohN, uint32_t ch)
{
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

    return;
}

void ttcGenConfLocal(localArgs * la, uint32_t L1Ainterval, uint32_t pulseDelay)
{
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.RESET", 0x1, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.ENABLE", 0x1, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_GAP", L1Ainterval, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_CALPULSE_TO_L1A_GAP", pulseDelay, la->response);
    return;
}

void ttcGenConf(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t L1Ainterval = request->get_word("L1Ainterval");
    uint32_t pulseDelay = request->get_word("pulseDelay");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    ttcGenConfLocal(&la, L1Ainterval, pulseDelay);

    return;

}

void genScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask, uint32_t ch, uint32_t enCal, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, std::string scanReg)
{

    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", nevts, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x1, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.SINGLE_RESYNC", 0x1, la->response);
    dacMonConfLocal(la, ohN, 0);
    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    uint32_t notmask = ~mask & 0xFFFFFF;
    char regBuf[200];
    if( (notmask & goodVFATs) != notmask) 
    { 
        sprintf(regBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        la->response->set_string("error",regBuf); 
        return;
    }

    if(ch >= 128)
    {
        if(enCal)
        {
            la->response->set_string("error","Brian, it doesn't make sense to calpulse all channels"); 
            return;
        }
    }
    else
    {
        for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
        {
            //uint32_t val = readRawReg(la->rtxn, la->dbi, regBuf, la->response);
            //if(vfatCH == ch && enCal) val = (val | 0x8000);
            //else val = (val & 0xFFFF7FFF);
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
            writeReg(la->rtxn, la->dbi, regBuf, 0x1, la->response);
        }
    }

    for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
    { 
        sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,vfatN); 
        writeReg(la->rtxn, la->dbi, regBuf, 0x1, la->response);
    }

    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep)
    {
        for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
        { 
            sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_%s",ohN,vfatN,scanReg.c_str()); 
            writeReg(la->rtxn, la->dbi, regBuf, dacVal, la->response);
        }
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_START", 0x1, la->response);
        bool running = true;
        while(running) running = readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_RUNNING");
        for(int vfatN = 0; vfatN < 24; vfatN++)
        { 
            int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
            sprintf(regBuf,"GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN);
            outData[idx] = readRawReg(la->rtxn, la->dbi, regBuf, la->response);
        }

    }

    for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
    { 
        sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,vfatN); 
        writeReg(la->rtxn, la->dbi, regBuf, 0x0, la->response);
        sprintf(regBuf,"GEM_AMC.OH.OH%i.GEB.VFAT%i.VFAT_CHANNELS.CHANNEL%i.CALPULSE_ENABLE", ohN, vfatN, ch);
        if(ch < 128) writeReg(la->rtxn, la->dbi, regBuf, 0x1, la->response);
    }
}

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
    uint32_t enCal = request->get_word("enCal");
    std::string scanReg = request->get_string("scanReg");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*(dacMax-dacMin+1)/dacStep];
    genScanLocal(&la, outData, ohN, mask, ch, enCal, nevts, dacMin, dacMax, dacStep, scanReg);
    response->set_word_array("data",outData,24*(dacMax-dacMin+1)/dacStep);

}

void scurveScanLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, RPCMsg *response)
{
}

void scurveScan(const RPCMsg *request, RPCMsg *response)
{
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
        modmgr->register_method("calibration_routines", "scurveScan", scurveScan);
        modmgr->register_method("calibration_routines", "ttcGenConf", ttcGenConf);
    }
}
