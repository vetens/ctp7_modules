#include <pthread.h>
#include "optohybrid.h"

struct localArgs {
    lmdb::txn & rtxn;
    lmdb::dbi & dbi;
    RPCMsg *response;
};

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

uint32_t vfatSyncCheckLocal(localArgs * la, uint32_t ohN)
{
    uint32_t goodVFATs = 0;
    for(int vfatN = 0; vfatN < 24; vfatN++)
    {
        char regBase [100];
        sprintf(regBase, "GEM_AMC.OH_LINKS.OH%i.VFAT%i",ohN, vfatN);
        bool linkGood = readReg(la->rtxn, la->dbi, std::string(regBase)+".LINK_GOOD");
        uint32_t linkErrors = readReg(la->rtxn, la->dbi, std::string(regBase)+".SYNC_ERR_CNT");
        goodVFATs = goodVFATs | ((linkGood && (linkErrors == 0)) << vfatN);
    }

    return goodVFATs;
}

void vfatSyncCheck(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t goodVFATs = vfatSyncCheckLocal(&la, ohN);

    response->set_word("goodVFATs", goodVFATs);

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

void thresholdScanLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t mask,uint32_t ch, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep, RPCMsg *response)
{
    
    writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_L1A_COUNT", nevts, la->response);
    writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x1, la->response);
    dacMonConfLocal(la, ohN, ch);
    uint32_t goodVFATs = vfatSyncCheckLocal(la, ohN);
    uint32_t notmask = ~mask & 0xFFFFFF;
    if( (notmask & goodVFATs) != notmask) 
    { 
        char errBuf[200];
        sprintf(errBuf,"One of the unmasked VFATs is not Synced. goodVFATs: %x\tnotmask: %x",goodVFATs,notmask);
        response->set_string("error",errBuf); 
        return;
    }
    char regName[100];
    for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
    { 
        sprintf(regName,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_RUN",ohN,vfatN); 
        writeReg(la->rtxn, la->dbi, regName, 0x1, la->response);
        sprintf(regName,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_THR_ARM_DAC",ohN,vfatN); 
        writeReg(la->rtxn, la->dbi, regName, 0, la->response);
    }
    for(uint32_t dacVal = dacMin; dacVal <= dacMax; dacVal += dacStep)
    {
        for(int vfatN = 0; vfatN < 24; vfatN++) if((notmask >> vfatN) & 0x1)
        { 
            sprintf(regName,"GEM_AMC.OH.OH%i.GEB.VFAT%i.CFG_THR_ZCC_DAC",ohN,vfatN); 
            writeReg(la->rtxn, la->dbi, regName, dacVal, la->response);
        }
        writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.CTRL.RESET", 0x1, la->response);
        writeReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_START", 0x1, la->response);
        bool running = true;
        while(running) running = readReg(la->rtxn, la->dbi, "GEM_AMC.TTC.GENERATOR.CYCLIC_RUNNING");
        for(int vfatN = 0; vfatN < 24; vfatN++)
        { 
            char regBuf[200];
            int idx = vfatN*(dacMax-dacMin+1)/dacStep+(dacVal-dacMin)/dacStep;
            sprintf(regBuf,"GEM_AMC.GEM_TESTS.VFAT_DAQ_MONITOR.VFAT%i.GOOD_EVENTS_COUNT",vfatN);
            outData[idx] = readRawReg(la->rtxn, la->dbi, regBuf, la->response);
        }

    }
    writeReg(la->rtxn, la->dbi, "GEM_AMC.GEM_SYSTEM.VFAT3.VFAT3_RUN_MODE", 0x0, la->response);
}

void thresholdScan(const RPCMsg *request, RPCMsg *response)
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

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t outData[24*(dacMax-dacMin+1)/dacStep];
    thresholdScanLocal(&la, outData, ohN, mask, ch, nevts, dacMin, dacMax, dacStep, response);
    response->set_word_array("data",outData,24*(dacMax-dacMin+1)/dacStep);

}

void *latencyScanLocal(void * args)
{
    struct localArgs *local_args = (struct localArgs *) args; 
    startScanModule(local_args->rtxn, local_args->dbi, local_args->response);
}

void latencyScan(const RPCMsg *request, RPCMsg *response)
{
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    env.open("/mnt/persistent/texas/address_table.mdb", 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    pthread_t thread;
    configureScanModule(rtxn, dbi, request, response);
    struct localArgs args = {.rtxn = rtxn, .dbi = dbi, .response = response};
    pthread_create(&thread,NULL,&latencyScanLocal,(void *)&args);

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
        modmgr->register_method("calibration_routines", "thresholdScan", thresholdScan);
        modmgr->register_method("calibration_routines", "latencyScan", latencyScan);
        modmgr->register_method("calibration_routines", "scurveScan", scurveScan);
        modmgr->register_method("calibration_routines", "ttcGenConf", ttcGenConf);
    }
}
