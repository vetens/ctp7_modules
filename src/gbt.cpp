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
    }
}

