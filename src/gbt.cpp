/*! \file
 *  \brief RPC module for GBT methods
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#include "gbt.h"
#include "gbt_constants.h"

#include "moduleapi.h"
#include "memhub.h"
#include "utils.h"

/*! \brief This macro is used to terminate a function if an error occurs. It logs the message, write it to the `error` RPC key and returns the `error_code` value.
 *  \param response A pointer to the RPC response object.
 *  \param message The `std::string` error message.
 *  \param error_code Value which is passed to the `return` statement.
 */
#define EMIT_RPC_ERROR(response, message, error_code){ \
    LOGGER->log_message(LogManager::ERROR, message); \
    response->set_string("error", message); \
    return error_code; }

void writeGBTConfig(const RPCMsg *request, RPCMsg *response){
    auto env = lmdb::env::create();
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */
    std::string gem_path = std::getenv("GEM_PATH");
    std::string lmdb_data_file = gem_path+"/address_table.mdb";
    env.open(lmdb_data_file.c_str(), 0, 0664);
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);
    auto dbi = lmdb::dbi::open(rtxn, nullptr);

    struct localArgs la = {.rtxn = rtxn, .dbi = dbi, .response = response};

    // ohN key
    const uint32_t ohN = request->get_word("ohN");
    const uint32_t ohMax = readReg(&la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(response, stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), )

    // gbtN key
    const uint32_t gbtN = request->get_word("gbtN");
    if (gbtN >= gbt::gbtsPerOH)
        EMIT_RPC_ERROR(response, stdsprintf("The gbtN parameter supplied (%u) exceeds the number of GBT's per OH (%u).", gbtN, gbt::gbtsPerOH),)

    // config key
    const uint32_t configSize = request->get_binarydata_size("config");
    if (configSize != gbt::configSize)
        EMIT_RPC_ERROR(response, stdsprintf("The provided configuration has not the correct size. It is %u registers long while this methods expects %hu 8-bits registers.", configSize, gbt::configSize), );

    gbt::config_t config{};
    request->get_binarydata("config", config.data(), config.size());

    // Write the configuration
    writeGBTConfigLocal(&la, ohN, gbtN, config);

    return;
} //End writeGBTConfig(...)

bool writeGBTConfigLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const gbt::config_t &config){
    LOGGER->log_message(LogManager::INFO, stdsprintf("Writing the configuration of OH #%u - GBTX #%u.", ohN, gbtN));

    // ohN check
    const uint32_t ohMax = readReg(la, "GEM_AMC.GEM_SYSTEM.CONFIG.NUM_OF_OH");
    if (ohN >= ohMax)
        EMIT_RPC_ERROR(la->response, stdsprintf("The ohN parameter supplied (%u) exceeds the number of OH's supported by the CTP7 (%u).", ohN, ohMax), true)

    // gbtN check
    if (gbtN >= gbt::gbtsPerOH)
        EMIT_RPC_ERROR(la->response, stdsprintf("The gbtN parameter supplied (%u) exceeds the number of GBT's per OH (%u).", gbtN, gbt::gbtsPerOH), true)

    // Write all the registers
    for (size_t address = 0; address < gbt::configSize; address++){
        if (writeGBTRegLocal(la, ohN, gbtN, static_cast<uint16_t>(address), config[address]))
            return true;
    }

    return false;
} //End writeGBTConfigLocal(...)

bool writeGBTRegLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value){
    // Check that the GBT exists
    if (gbtN >= gbt::gbtsPerOH)
        EMIT_RPC_ERROR(la->response, stdsprintf("The gbtN parameter supplied (%u) is larger than the number of GBT's per OH (%u).", gbtN, gbt::gbtsPerOH), true)

    // Check that the address is writable
    if (address >= gbt::configSize)
        EMIT_RPC_ERROR(la->response, stdsprintf("GBT has %hu writable addresses while the provided address is %hu.", gbt::configSize-1, address), true)

    // GBT registers are 8 bits long
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.READ_WRITE_LENGTH", 1);

    // Select the link number
    const uint32_t linkN = ohN*gbt::gbtsPerOH + gbtN;
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.GBTX_LINK_SELECT", linkN);

    // Write to the register
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.ADDRESS", address);
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.WRITE_DATA", value);
    writeReg(la, "GEM_AMC.SLOW_CONTROL.IC.EXECUTE_WRITE", 1);

    return false;
} //End writeGBTRegLocal(...)

extern "C" {
    const char *module_version_key = "gbt v1.0.1";
    int module_activity_color = 4;
    void module_init(ModuleManager *modmgr) {
        if (memhub_open(&memsvc) != 0) {
            LOGGER->log_message(LogManager::ERROR, stdsprintf("Unable to connect to memory service: %s", memsvc_get_last_error(memsvc)));
            LOGGER->log_message(LogManager::ERROR, "Unable to load module");
            return; // Do not register our functions, we depend on memsvc.
        }
        modmgr->register_method("gbt", "writeGBTConfig", writeGBTConfig);
    }
}

