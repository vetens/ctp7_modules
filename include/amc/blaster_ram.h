/*! \file include/amc/blaster_ram.h
 *  \brief AMC BLASTER RAM methods for RPC modules
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_H
#define AMC_H

#include "utils.h"

/*!
 *  \brief Writes configuration `BLOB` to BLASTER RAM
 *
 *  The CTP7 has three RAMs that store configuration information for `GBT`, `OptoHybrid`, and `VFAT`
 *  The `BLOB` provided here will be the configuration blob for each of the three RAMs, as described in the specific config writing functions
 *
 *  \param la Local arguments structure
 *  \param blob configuration `BLOB`
 */
bool writeConfRAMLocal(localArgs *la, uint32_t* blob);

/*!
 *  \brief Writes configuration `BLOB` to BLASTER GBT_RAM
 *
 *  \detail The CTP7 has a RAM that stores configuration information for all GBTxs connected to the card.
 *  * The `BLOB` is a sequence of 8-bit register values for each GBT register (366 total).
 *  * The 8 bit configuration for GBT register 0 should be written to the lowest byte.
 *  * Each subsequent register fills the next byte.
 *  * GBT0 for OH0 is first, followed by GBT1, and then GBT2.
 *  * This is repeated for OH1...OHN, or as specified in the `linkMask`
 *
 *  \param la Local arguments structure
 *  \param gbtblob GBT configuration `BLOB` corresponding to all GBTs on all listed links
 *  \param linkMask links for which to fill the configuration
 */
bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob, uint16_t linkMask=0xfff);

/*!
 *  \brief Writes configuration `BLOB` to BLASTER OH_RAM
 *
 *  \detail The CTP7 has a RAM that stores all configuration information for all OptoHybrids connected to the card.
 *  * The `BLOB` is a sequence of pairs of 32-bit register addresses followed by 32-bit register values for each OH register.
 *  * The local OH address for the first register in OH0 is written to the lowest 32 bits, followed by the value to be written to that register.
 *  * Subsequent bits are allocated for the subsequent address/value pairs, and then repeated by the same for OH1...OHN, or as specified in the `linkMask`
 *
 *  \param la Local arguments structure
 *  \param ohblob OptoHybrid configuration `BLOB` corresponding to all OptoHybrids on all listed links
 *  \param linkMask links for which to fill the configuration
 */
bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob, uint16_t linkMask=0xfff);


/*!
 *  \brief Writes configuration `BLOB` to BLASTER VFAT_RAM
 *
 *  \detail The CTP7 has a RAM that stores configuration information for all VFATs connected to the card.
 *  * The `BLOB` is a sequence of 16-bit register values for each VFAT register (147 total, the MS16-bits are ignored, but must be sent).
 *  * The 16 bit configuration for VFAT0 register 0 should be written to the 16 lowest bits.
 *  * Each subsequent register fills the next 16 bits, until register 147, which should then be followed by 16 0's
 *  * This is then repeated for OH1...OHN, or as specified in the `linkMask`
 *
 *  \param la Local arguments structure
 *  \param vfatblob VFAT configuration `BLOB` corresponding to all VFATs on all listed links
 *  \param linkMask links for which to fill the configuration
 */
bool writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob, uint16_t linkMask=0xfff);

/** RPC callbacks */
void writeConfRAM(const RPCMsg *request, RPCMsg *response);
void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response);
void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response);
void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response);

#endif
