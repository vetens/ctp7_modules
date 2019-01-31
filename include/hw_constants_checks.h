/*! \file
 *  \brief Header containing helper functions to check the hardware related constants.
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef HW_CONSTANTS_CHECKS_H
#define HW_CONSTANTS_CHECKS_H

#include "hw_constants.h"
#include "wiscRPCMsg.h"

/*! \brief This namespace hold the check for constants related to the GBT.
 */
namespace gbt{
    /*! \brief This function checks the phase parameter validity.
     *  \param[in, out] response Pointer to the RPC response object.
     *  \param[in] phase Phase value to check.
     *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `error` RPC key.
     */
    inline bool checkPhase(wisc::RPCMsg *response, uint8_t phase){
        if (phase < gbt::PHASE_MIN)
            EMIT_RPC_ERROR(response, stdsprintf("The phase parameter supplied (%hhu) is smaller than the minimal phase (%hhu).", phase, gbt::PHASE_MIN), true)
        if (phase > gbt::PHASE_MAX)
            EMIT_RPC_ERROR(response, stdsprintf("The phase parameter supplied (%hhu) is bigger than the maximal phase (%hhu).", phase, gbt::PHASE_MAX), true)
        return false;
    }
}

#endif

