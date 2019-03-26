/*! \file include/amc/blaster_ram.h
 *  \brief AMC BLASTER RAM methods for RPC modules
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_BLASTER_RAM_H
#define AMC_BLASTER_RAM_H

#include "utils.h"
#include "amc/blaster_ram_defs.h"

/*!
 *  \brief Returns the size of the specified RAM in the BLASTER module
 *
 *  \param la Local arguments structure
 *  \param type Select which RAM to obtain the size of
 *  \returns size of the RAM in 32-bit words
 */
uint32_t getRAMMaxSize(localArgs *la, BLASTERTypeT const& type);

/*!
 *  \brief Verify the size of the provided BLOB size for a specified RAM in the BLASTER module
 *
 *  \param la Local arguments structure
 *  \param type Select which RAM to obtain the size of
 *  \param sz Size of the BLOB that will be written
 *  \returns true if the size is correct for the specified RAM
 */
bool getRAMMaxSize(localArgs *la, BLASTERTypeT const& type, size_t const& sz);

/*!
 *  \brief Extract the starting address of the RAM for a specified component
 *
 *  \param la Local arguments structure
 *  \param ohN Select which OptoHybrid the component is associated with (0--11)
 *  \param partN Select which VFAT/GBTx the configuration is for
 *         0--23 (VFAT)
 *         0--2 (GBTx)
 *  \returns address in the block RAM specified
 */
uint32_t getRAMBaseAddr(localArgs *la, BLASTERTypeT const& type, uint8_t const& ohN, uint8_t const& partN);

/**
   read functions
**/

/*!
 *  \brief Reads configuration `BLOB` from BLASTER RAM
 *
 *  The CTP7 has three RAMs that store configuration information for `GBT`, `OptoHybrid`, and `VFAT`
 *  The `BLOB` provided here will conatin the configuration blob for one (or all) of the three RAMs
 *
 *  \param la Local arguments structure
 *  \param type specifies which of the RAMs to read, options include:
 *         BLASTERType::GBT - will return all GBT configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::OptoHybrid - will return all OptoHybrid configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::VFAT - will return all VFAT configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::ALL - will return the full configuration of a single CTP7 BLASTER RAM
 *  \param blob to store the configuration `BLOB`
 *  \param blob_sz number of 32-bit words in configuration `BLOB`.
 *         Must be equal to the size of the RAM specified:
 *         * if the size is more, an error will be thrown
 *         * if the size is less, only the number of words specified will be read from the RAM (maybe)
 *         * if ALL is specified, the size must be the total RAM size
 *           - GBT_RAM_SIZE*N_GBTX*N_OH + OH_RAM_SIZE*N_OH + VFAT_RAM_SIZE*N_VFAT*N_OH
 *         Must not exceed GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE +
 *                         GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE +
 *                         GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 */
uint32_t readConfRAMLocal(localArgs *la, BLASTERTypeT const& type, uint32_t* blob, size_t const& blob_sz);

/*!
 *  \brief Reads GBT configuration `BLOB` from BLASTER GBT_RAM for specified OptoHybrid (3 GBT BLOBs)
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
 *         If a link is not specified, that portion of the RAM will be skipped
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 *  \returns Number of GBT BLOB words read in 32-bit words
 */
uint32_t readGBTConfRAMLocal(localArgs *la, void* gbtblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);

/*!
 *  \brief Reads OptoHybrid configuration `BLOB` from BLASTER OH_RAM
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
 *         If a link is not specified, that portion of the RAM will be skipped
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 *  \returns Number of OptoHybrid BLOB words read in 32-bit words
 */
uint32_t readOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);


/*!
 *  \brief Reads VFAT configuration `BLOB` from BLASTER VFAT_RAM for specified OptoHybrid (24 VFAT BLOBs)
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
 *         Should be equal to VFAT_RAM_SIZE*N_VFAT*N_OH
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 *  \param ohMask links for which to fill the configuration.
 *         If a link is not specified, that portion of the RAM will be skipped/skipped?
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 *  \returns Number of VFAT BLOB words read in 32-bit words
 */
uint32_t readVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);


/**
   write functions
**/

/*!
 *  \brief Writes configuration `BLOB` to BLASTER RAM
 *
 *  The CTP7 has three RAMs that store configuration information for `GBT`, `OptoHybrid`, and `VFAT`
 *  The `BLOB` provided here contains the configuration blob for one (or all) of the three RAMs
 *
 *  \param la Local arguments structure
 *  \param type specifies which of the RAMs to write, options include:
 *         BLASTERType::GBT - will write all GBT configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::OptoHybrid - will write all OptoHybrid configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::VFAT - will write all VFAT configurations for a single CTP7 BLASTER RAM
 *         BLASTERType::ALL - will write the full configuration of a single CTP7 BLASTER RAM
 *  \param blob configuration `BLOB`
 *  \param blob_sz number of 32-bit words in configuration `BLOB`.
 *         Must be equal to the size of the RAM specified:
 *         * if the size is more, an error will be thrown
 *         * if the size is less, only the number of words specified will be written to the RAM (maybe)
 *         * if ALL is specified, the size must be the total RAM size
 *           - GBT_RAM_SIZE*N_GBTX*N_OH + OH_RAM_SIZE*N_OH + VFAT_RAM_SIZE*N_VFAT*N_OH
 *         Must not exceed GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE +
 *                         GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE +
 *                         GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 */
void writeConfRAMLocal(localArgs *la, BLASTERTypeT const& type, uint32_t* blob, size_t const& blob_sz);

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
 *         If a link is not specified, that portion of the RAM will be skipped
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 */
void writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);

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
 *         If a link is not specified, that portion of the RAM will be skipped
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 */
void writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);


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
 *         Should be equal to VFAT_RAM_SIZE*N_VFAT*N_OH
 *         Should not exceed GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
 *  \param ohMask links for which to fill the configuration.
 *         If a link is not specified, that portion of the RAM will be skipped/skipped?
 *         WARNING: `ohMask` assumes that the BLOB structure skips the masked links
 *         Default value is 0xfff, for all GE1/1 OptoHybrids, a value of 0x0 will be treated the same
 *         Other values in the range (0x0,0xfff) will be treated as described
 */
void writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask=0xfff);

/*!
   \brief BLASTER RAM RPC callbacks
 */

/*!
   \param[in] "type" type of BLASTER RAM configration provided
   \param[out] "confblob" binary data blob containing the configuration read
 */
void readConfRAM(const RPCMsg *request, RPCMsg *response);

/*!
   \param[in] "type" type of BLASTER RAM configration provided
   \param[in] "confblob" binary data blob containing the configuration to be written
 */
void writeConfRAM(const RPCMsg *request, RPCMsg *response);

/*!
   \param[in] "gbtblob" binary data blob containing the GBT configuration to be written
 */
void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response);

/*!
   \param[in] "ohblob" binary data blob containing the OptoHybrid configuration to be written
 */
void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response);

/*!
   \param[in] "vfatblob" binary data blob containing the VFAT configuration to be written
 */
void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response);

#endif
