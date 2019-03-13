/*! \file include/amc/blaster_ram.h
 *  \brief AMC BLASTER RAM methods for RPC modules
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_H
#define AMC_H

#include "utils.h"

/*!
 *  \brief Extract the starting address of the RAM for a specified GBT/OptoHybrid combination
 *
 *  \param la Local arguments structure
 *  \param ohN Select which OptoHybrid the GBT is associated with (0--11)
 *  \param gbtN Select which GBTx the configuration is for 0--2
 *  \returns address in the block RAM specified
 */
uint32_t getGBTRAMBaseAddr(localArgs *la, uint8_t const& ohN, uint8_t const& gbtN);

/*!
 *  \brief Extract the starting address of the RAM for a specified OptoHybrid combination
 *
 *  \param la Local arguments structure
 *  \param ohN Select which OptoHybrid the OptoHybrid is associated with (0--11)
 *  \returns address in the block RAM specified
 */
uint32_t getOptoHybridRAMBaseAddr(localArgs *la, uint8_t const& ohN);

/*!
 *  \brief Extract the starting address of the RAM for a specified VFAT/OptoHybrid combination
 *
 *  \param la Local arguments structure
 *  \param ohN Select which OptoHybrid the VFAT is associated with (0--11)
 *  \param vfatN Select which VFAT the configuration is for 0--23
 *  \returns address in the block RAM specified
 */
uint32_t getVFATRAMBaseAddr(localArgs *la, uint8_t const& ohN, uint8_t const& vfatN);

/*!
 *  \brief Writes configuration `BLOB` to BLASTER RAM
 *
 *  The CTP7 has three RAMs that store configuration information for `GBT`, `OptoHybrid`, and `VFAT`
 *  The `BLOB` provided here will be the configuration blob for each of the three RAMs, as described in the specific config writing functions
 *
 *  \param la Local arguments structure
 *  \param blob configuration `BLOB`
 *  \param blob_sz number of 32-bit words in configuration `BLOB`.
 *         Should be equal to GBT_RAM_SIZE*N_GBTX*N_OH + OH_RAM_SIZE*N_OH + VFAT_RAM_SIZE*N_VFAT
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE +
 *                           GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE +
 *                           GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 */
bool writeConfRAMLocal(localArgs *la, uint32_t* blob, size_t const& blob_sz);

/*!
 *  \brief Reads configuration `BLOB` from BLASTER RAM
 *
uint32_t readConfRAMLocal(localArgs *la, uint32_t* blob);
 */

/*!
 *  \brief Writes configuration `BLOB` to BLASTER GBT_RAM
 *
 *  \detail The CTP7 has a RAM that stores configuration information for all GBTxs connected to the card.
 *          The `BLOB` is a sequence of 8-bit register values for each GBT register (366 total).
 *          The 8 bit configuration for GBT register 0 should be written to the lowest byte.
 *          Each subsequent register fills the next byte.
 *          GBT0 for OH0 is first, followed by GBT1, and then GBT2.
 *          This is repeated for OH1...OHN, or as specified in the `ohMask`
 *
 *  \param la Local arguments structure
 *  \param gbtblob GBT configuration `BLOB` corresponding to all GBTs on all listed links
 *  \param blob_sz number of 32-bit words in GBT configuration `BLOB`
 *         Should be equal to GBT_RAM_SIZE*N_GBTX*N_OH
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE
 *  \param ohMask links for which to fill the configuration.
 *         If a link is not specified, that portion of the RAM will be filled with 0's
 */
bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);

/*!
 *  \brief Writes configuration `BLOB` to BLASTER OH_RAM
 *
 *  \detail The CTP7 has a RAM that stores all configuration information for all OptoHybrids connected to the card.
 *          The `BLOB` is a sequence of pairs of 32-bit register addresses followed by 32-bit register values for each OH register.
 *          The local OH address for the first register in OH0 is written to the lowest 32 bits, followed by the value to be written to that register.
 *          Subsequent bits are allocated for the subsequent address/value pairs, and then repeated by the same for OH1...OHN, or as specified in the `ohMask`
 *
 *  \param la Local arguments structure
 *  \param ohblob OptoHybrid configuration `BLOB` corresponding to all OptoHybrids on all listed links
 *  \param blob_sz number of 32-bit words in OptoHybrid configuration `BLOB`
 *         Should be equal to OH_RAM_SIZE*N_OH
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE
 *  \param ohMask links for which to fill the configuration.
 *         If a link is not specified, that portion of the RAM will be filled with 0's
 */
bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);


/*!
 *  \brief Writes configuration `BLOB` to BLASTER VFAT_RAM
 *
 *  \detail The CTP7 has a RAM that stores configuration information for all VFATs connected to the card.
 *          The `BLOB` is a sequence of 16-bit register values for each VFAT register (147 total, the MS16-bits are ignored, but must be sent).
 *          The 16 bit configuration for VFAT0 register 0 should be written to the 16 lowest bits.
 *          Each subsequent register fills the next 16 bits, until register 147, which should then be followed by 16 0's
 *          This is then repeated for OH1...OHN, or as specified in the `ohMask`
 *
 *  \param la Local arguments structure
 *  \param vfatblob VFAT configuration `BLOB` corresponding to all VFATs on all listed links
 *  \param blob_sz number of 32-bit words in VFAT configuration `BLOB`
 *         Should be equal to VFAT_RAM_SIZE*N_VFAT
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 *  \param ohMask links for which to fill the configuration.
 *         If a link is not specified, that portion of the RAM will be filled with 0's/skipped?
 */
bool writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);

/** RPC callbacks */
void writeConfRAM(const RPCMsg *request, RPCMsg *response);
void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response);
void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response);
void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response);

#endif
