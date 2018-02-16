#include "utils.h"
#include <string>

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
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    uint32_t ohN = request->get_word("ohN");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    uint32_t goodVFATs = vfatSyncCheckLocal(&la, ohN);

    response->set_word("goodVFATs", goodVFATs);

    return;

}

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
                writeReg(la->rtxn, la->dbi, regName, dacVal, la->response);
            }
        }
    }
}

void configureVFAT3s(const RPCMsg *request, RPCMsg *response) {
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"address_table.mdb/data.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);
    uint32_t ohN = request->get_word("ohN");
    uint32_t vfatMask = request->get_word("vfatMask");
    LOGGER->log_message(LogManager::INFO, "Load VFAT configuration");

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};
    configureVFAT3sLocal(&la, ohN, vfatMask);
    rtxn.abort();
}

