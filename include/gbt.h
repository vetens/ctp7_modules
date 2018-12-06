/*! \file
 *  \brief RPC module for GBT methods
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef GBT_H
#define GBT_H

#include "utils.h"

/*! \brief This function writes a single register in the given GBT of the given OptoHybrid.
 *  \param[in, out] la Local arguments structure.
 *  \param[in] ohN OptoHybrid index number. Warning : the value of this parameter is not checked because of the cost of a register access.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] address GBT register address to write. The highest writable address is 365.
 *  \param[in] value Value to write to the GBT register.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `error` RPC key.
 */
bool writeGBTRegLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const uint16_t address, const uint8_t value);

#endif

