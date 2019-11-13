/*! \file
 *  \brief Header containing helper functions to check the hardware related constants.
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef HW_CONSTANTS_CHECKS_H
#define HW_CONSTANTS_CHECKS_H

#include "hw_constants.h"

/*! \brief This namespace hold the check for constants related to the GBT.
 */
namespace gbt{
    /*! \brief This function checks the phase parameter validity.
     *  \param[in, out] response Pointer to the RPC response object.
     *  \param[in] phase Phase value to check.
     *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `error` RPC key.
     */
    inline void checkPhase(uint8_t phase){
        if (phase < PHASE_MIN)
            std::stringstream errmsg;
            errmsg.clear();
            errmsg.str("");
            errmsg << "The phase parameter supplied (" << phase << ") is smaller than the minimal phase (" << PHASE_MIN << ").";
            throw std::range_error(errmsg)
        if (phase > PHASE_MAX)
            std::stringstream errmsg;
            errmsg.clear();
            errmsg.str("");
            errmsg << "The phase parameter supplied (" << phase << ") is bigger than the maximal phase (" << PHASE_MAX << ").";
            throw std::range_error(errmsg)
        return;
    }
}

#endif
