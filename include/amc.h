/*! \file amc.h
 *  \brief RPC module for optohybrid methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Brin Dorney <brian.l.dorney@cern.ch>
 */

#ifndef AMC_H
#define AMC_H

#include "utils.h"
#include <unistd.h>

/*! \fn unsigned int fw_version_check(const char* caller_name, localArgs *la)
 *  \brief Returns AMC FW version
 *  in case FW version is not 1.X or 3.X sets an error string in response
 *  \param caller_name Name of methods which called the FW version check
 *  \param la Local arguments structure
 */
unsigned int fw_version_check(const char* caller_name, localArgs *la);

/*! \fn void getmonDAQmainLocal(localArgs * la)
 *  \brief Local version of getmonDAQmain
 *  \param la Local arguments
 */
void getmonDAQmainLocal(localArgs * la);

/*! \fn void getmonDAQmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */

void getmonDAQmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonDAQOHmainLocal(localArgs * la, int NOH, int ohMask)
 *  \brief Local version of getmonDAQOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonDAQOHmainLocal(localArgs * la, int NOH=12, int ohMask=0xfff);

/*! \fn void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of DAQ monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonDAQOHmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonOHmainLocal(localArgs * la, int NOH, int ohMask)
 *  \brief Local version of getmonOHmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonOHmainLocal(localArgs * la, int NOH=12, int ohMask=0xfff);

/*! \fn void getmonOHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of OH monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonOHSCAmainLocal(localArgs *la, int NOH, int ohMask)
 *  \brief Local version of getmonOHSCAmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonOHSCAmainLocal(localArgs *la, int NOH=12, int ohMask=0xfff);

/* !\fn void getmonOHSCAmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads the SCA Monitoring values of all OH's (voltage and temperature)
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHSCAmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonOHSysmonLocal(localArgs *la, int NOH=12, int ohMask=0xfff)
 *  \brief Local version of getmonOHSysmon
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonOHSysmonLocal(localArgs *la, int NOH=12, int ohMask=0xfff);

/*! \fn void getmonOHSysmon(const RPCMsg *request, RPCMsg *response)
 *  \brief reads FPGA Sysmon values of all unmasked OH's
 *  \details Reads FPGA core temperature, core voltage (1V), and I/O voltage (2.5V).  Will also check error conditions (over temperature, 1V VCCINT, and 2.5V VCCAUX), and the error conunters for those conditions.
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHSysmon(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonTRIGGERmainLocal(localArgs * la, int NOH, int ohMask)
 *  \brief Local version of getmonTRIGGERmain
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonTRIGGERmainLocal(localArgs * la, int NOH=12, int ohMask=0xfff);

/*! \fn void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTRIGGERmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonTRIGGEROHmainLocal(localArgs * la, int NOH, int ohMask)
 *  \brief Local version of getmonTRIGGEROHmain
 *  * LINK{0,1}_SBIT_OVERFLOW_CNT -- this is an interesting counter to monitor from operations perspective, but is not related to the health of the link itself. Rather it shows how many times OH had too many clusters which it couldn't fit into the 8 cluster per BX bandwidth. If this counter is going up it just means that OH is seeing a very high hit occupancy, which could be due to high radiation background, or thresholds configured too low.
 *
 *  The other 3 counters reflect the health of the optical links:
 *  * LINK{0,1}_OVERFLOW_CNT and LINK{0,1}_UNDERFLOW_CNT going up indicate a clocking issue, specifically they go up when the frequency of the clock on the OH doesn't match the frequency on the CTP7. Given that CTP7 is providing the clock to OH, this in principle should not happen unless the OH is sending complete garbage and thus the clock cannot be recovered on CTP7 side, or the bit-error rate is insanely high, or the fiber is just disconnected, or OH is off. In other words, this indicates a critical problem.
 *  * LINK{0,1}_MISSED_COMMA_CNT also monitors the health of the link, but it's more sensitive, because it can go up due to isolated single bit errors. With radiation around, this might count one or two counts in a day or two. But if it starts running away, that would indicate a real problem, but in this case most likely the overflow and underflow counters would also see it.
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 */
void getmonTRIGGEROHmainLocal(localArgs * la, int NOH=12, int ohMask=0xfff);

/*! \fn void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of trigger monitoring registers at the OH
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTRIGGEROHmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonTTCmainLocal(localArgs * la)
 *  \brief Local version of getmonTTCmain
 *  \param la Local arguments
 */
void getmonTTCmainLocal(localArgs * la);

/*! \fn void getmonTTCmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Reads a set of TTC monitoring registers
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonTTCmain(const RPCMsg *request, RPCMsg *response);

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

/*! \fn sbitReadOut(const RPCMsg *request, RPCMsg *response)
 *  \brief readout sbits using the SBIT Monitor.  See the local callable methods documentation for details.
 *  \param request RPC response message
 *  \param response RPC response message
 */
void sbitReadOut(const RPCMsg *request, RPCMsg *response);

#endif
