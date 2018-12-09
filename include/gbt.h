/*! \file
 *  \brief RPC module for GBT methods
 *  \author Laurent Pétré <lpetre@ulb.ac.be>
 */

#ifndef GBT_H
#define GBT_H

#include "gbt_constants.h"

#include "utils.h"

/*! \brief Write the GBT configuration of one OptoHybrid.
 *  \param[in] request RPC response message.
 *  \param[out] response RPC response message.
 *
 *  The method expects the following RPC keys :
 *  - `word ohN` : OptoHybrid index number.
 *  - `word gbtN` : Index of the GBT to configure. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  - `binarydata config` : Configuration blob of the GBT. This is a 366 element long vector of the registers to write. The registers are sorted from address 0 to address 365.
 *
 *  The method returns the following RPC keys :
 *  - `string error` : If an error occurs, this keys exists and contains the error message.
 */
void writeGBTConfig(const RPCMsg *request, RPCMsg *response);

/*! \brief Local callable version of `writeGBTConfig`
 *  \param[in, out] la Local arguments structure.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] gbtN Index of the GBT to write. There 3 GBT's per OptoHybrid in the GE1/1 chambers.
 *  \param[in] config Configuration blob of the GBT. This is a 366 elements long array whose each element is the value of one register sorted from address 0 to address 365.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `error` RPC key.
 */
bool writeGBTConfigLocal(localArgs *la, const uint32_t ohN, const uint32_t gbtN, const gbt::config_t &config);

/*! \brief Write the phase of a single VFAT.
 *  \param[in] request RPC response message.
 *  \param[out] response RPC response message.
 *
 *  The method expects the following RPC keys :
 *  - `word ohN` : OptoHybrid index number.
 *  - `word vfatN` : Index of the VFAT to configure. There 24 VFAT's per OptoHybrid in the GE1/1 chambers.
 *  - `word phase` : Phase value to write. The values span from 0 to 15.
 *
 *  The method returns the following RPC keys :
 *  - `string error` : If an error occurs, this keys exists and contains the error message.
 */
void writeGBTPhase(const RPCMsg *request, RPCMsg *response);

/*! \brief Local callable version of `writeGBTPhase`
 *  \param[in, out] la Local arguments structure.
 *  \param[in] ohN OptoHybrid index number.
 *  \param[in] vfatN VFAT index of which the phase is changed.
 *  \param[in] phase Phase value to write. Minimal phase is 0 and maximal phase is 15.
 *  \return Returns `false` in case of success; `true` in case of error. The precise error is logged and written to the `error` RPC key.
 */
bool writeGBTPhaseLocal(localArgs *la, const uint32_t ohN, const uint32_t vfatN, const uint8_t phase);

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

