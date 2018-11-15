/*! \file amc.h
 *  \brief RPC module for amc methods
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef AMC_H
#define AMC_H

#include "utils.h"

/*! \fn unsigned int fw_version_check(const char* caller_name, localArgs *la)
 *  \brief Returns AMC FW version
 *  in case FW version is not 1.X or 3.X sets an error string in response
 *  \param caller_name Name of methods which called the FW version check
 *  \param la Local arguments structure
 */
unsigned int fw_version_check(const char* caller_name, localArgs *la);

/*! \fn uint32_t getOHVFATMaskLocal(uint32_t ohN)
 *  \brief returns the vfatMask for the optohybrid ohN
 *  \details Reads the SYNC_ERR_CNT counter for each VFAT on ohN.  If for a given VFAT the counter returns a non-zero value the given VFAT will be masked.
 *  \param la Local arguments structure
 *  \param ohN Optical link
 */
uint32_t getOHVFATMaskLocal(localArgs * la, uint32_t ohN);

/*! \fn void getOHVFATMask(const RPCMsg *request, RPCMsg *response)
 *  \brief Determines the vfatMask for a given OH, see local method for details
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getOHVFATMask(const RPCMsg *request, RPCMsg *response);

/*! \fn void getOHVFATMaskMultiLink(const RPCMsg *request, RPCMsg *response)
 *  \brief As getOHVFATMask(...) but for all optical links specified in ohMask on the AMC
 *  \details Here the RPCMsg request should have a "ohMask" word which specifies which OH's to read from, this is a 12 bit number where a 1 in the n^th bit indicates that the n^th OH should be read back.  Additionally there should be a "ohVfatMaskArray" which is an array of size 12 where each element is the standard vfatMask for OH specified by the array index.
 *  \param request RPC request message
 *  \param response RPC responce message
 */
void getOHVFATMaskMultiLink(const RPCMsg *request, RPCMsg *response);

/*! \fn std::vector<uint32_t> sbitReadOutLocal(localArgs *la, uint32_t ohN, uint32_t acquireTime, bool *maxNetworkSizeReached)
 *  \brief reads out sbits from optohybrid ohN for a number of seconds given by acquireTime and returns them to the user.
 *  \details The SBIT Monitor stores the 8 SBITs that are sent from the OH (they are all sent at the same time and correspond to the same clock cycle). Each SBIT clusters readout from the SBIT Monitor is a 16 bit word with bits [0:10] being the sbit address and bits [12:14] being the sbit size, bits 11 and 15 are not used.
 *  \details The possible values of the SBIT Address are [0,1535].  Clusters with address less than 1536 are considered valid (e.g. there was an sbit); otherwise an invalid (no sbit) cluster is returned.  The SBIT address maps to a given trigger pad following the equation \f$sbit = addr % 64\f$.  There are 64 such trigger pads per VFAT.  Each trigger pad corresponds to two VFAT channels.  The SBIT to channel mapping follows \f$sbit=floor(chan/2)\f$.  You can determine the VFAT position of the sbit via the equation \f$vfatPos=7-int(addr/192)+int((addr%192)/64)*8\f$.
 *  \details The SBIT size represents the number of adjacent trigger pads are part of this cluster.  The SBIT address always reports the lowest trigger pad number in the cluster.  The sbit size takes values [0,7].  So an sbit cluster with address 13 and with size of 2 includes 3 trigger pads for a total of 6 vfat channels and starts at channel \f$13*2=26\f$ and continues to channel \f$(2*15)+1=31\f$.
 *  \details The output vector will always be of size N * 8 where N is the number of readouts of the SBIT Monitor.  For each readout the SBIT Monitor will be reset and then readout after 4095 clock cycles (~102.4 microseconds).  The SBIT clusters will only be added to the output vector if at least one of them was valid.  The SBIT clusters stored in the SBIT Monitor will not be over-written until a module reset is sent.  The readout will stop before acquireTime finishes if the size of the returned vector approaches the max TCP/IP size (~65000 btyes) and sets maxNetworkSize to true.
 *  \details Each element of the output vector will be a 32 bit word.  Bits [0,10] will the address of the SBIT Cluster, bits [11:13] will be the cluster size, and bits [14:26] will be the difference between the SBIT and the input L1A (if any) in clock cycles.  While the SBIT Monitor stores the difference between the SBIT and input L1A as a 32 bit number (0xFFFFFFFF) any value higher 0xFFF (12 bits) will be truncated to 0xFFF.  This matches the time between readouts of 4095 clock cycles.
 *  \param la Local arguments structure
 *  \param ohN Optical link
 *  \param acquireTime acquisition time on the wall clock in seconds
 *  \param maxNetworkSize pointer to a boolean, set to true if the returned vector reaches a byte count of 65000
 */
std::vector<uint32_t> sbitReadOutLocal(localArgs *la, uint32_t ohN, uint32_t acquireTime, bool *maxNetworkSizeReached);

/*! \fn void sbitReadOut(const RPCMsg *request, RPCMsg *response)
 *  \brief readout sbits using the SBIT Monitor.  See the local callable methods documentation for details.
 *  \param request RPC response message
 *  \param response RPC response message
 */
void sbitReadOut(const RPCMsg *request, RPCMsg *response);

/*! \fn bool writeConfRAMLocal(localArgs *la, uint32_t* blob)
 *  \brief Writes configuration BLOB to BLASTER RAM
 *
 *  The CTP7 has three RAMs that store configuration information for GBT, OptoHybrid, and VFAT
 *  The BLOB provided here will be the configuration blob for each of the three RAMs, as described in the specific config writing functions
 *
 *  \param la Local arguments structure
 *  \param blob configuration BLOB
 */
bool writeConfRAMLocal(localArgs *la, uint32_t* blob);

/*! \fn writeConfRAM(const RPCMsg *request, RPCMsg *response)
 *  \brief write configuration information into BLASTER RAM
 *  \param request RPC response message
 *  \param response RPC response message
 */
void writeConfRAM(const RPCMsg *request, RPCMsg *response);

/*! \fn bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob)
 *  \brief Writes configuration BLOB to BLASTER GBT_RAM
 *
 *  The CTP7 has a RAM that stores configuration information for all GBTxs connected to the card.
 *  The BLOB is a sequence of 8-bit register values for each GBT register (366 total).
 *  The 8 bit configuration for GBT register 0 should be written to the 8 lowest bits, with each successive register
 *  filling the next 8 bits.  GBT0 for OH0 is first, followed by GBT1, and then GBT2;
 *  then followed by the same for OH1...OHN
 *
 *  \param la Local arguments structure
 *  \param gbtblob GBT configuration BLOB
 */
bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob);

/*! \fn writeGBTConfRAM(const RPCMsg *request, RPCMsg *response)
 *  \brief write configuration information into BLASTER GBT_RAM
 *  \param request RPC response message
 *  \param response RPC response message
 */
void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response);

/*! \fn bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* blob)
 *  \brief Writes configuration BLOB to BLASTER OH_RAM
 *
 *  The CTP7 has a RAM that stores all configuration information for all OptoHybrids connected to the card
 *  The BLOB is a sequence of pairs of 32-bit register addresses followed by 32-bit register values for each OH register.
 *  The local OH address for the first register in OH0 is written to the lowest 32 bits, followed by the value to be written to that register
 *  Subsequent bits are allocated for the subsequent address/value pairs, and then repeated by the same for OH1...OHN
 *
 *  \param la Local arguments structure
 *  \param ohblob OptoHybrid configuration BLOB
 */
bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob);

/*! \fn writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response)
 *  \brief write configuration information into BLASTER OH_RAM
 *  \param request RPC response message
 *  \param response RPC response message
 */
void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response);

/*! \fn bool writeVFATConfRAMLocal(localArgs *la, uint32_t* blob)
 *  \brief Writes configuration BLOB to BLASTER VFAT_RAM
 *
 *  The CTP7 has a RAM that stores configuration information for all VFATs connected to the card.
 *  The BLOB is a sequence of 16-bit register values for each VFAT register (147 total, the MS16-bits are ignored, but must be filled).
 *  The 16 bit configuration for VFAT0 register 0 should be written to the 16 lowest bits, with each successive register
 *  filling the next 16 bits, until register 147, which will then be followed by 16 0's
 *  This is then repeated by the same for OH1...OHN
 *
 *  \param la Local arguments structure
 *  \param vfatblob VFAT configuration BLOB
 */
bool writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob);

/*! \fn writeVFATConfRAM(const RPCMsg *request, RPCMsg *response)
 *  \brief write configuration information into BLASTER VFAT_RAM
 *  \param request RPC response message
 *  \param response RPC response message
 */
void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response);

#endif
