/*! \file daq_monitor.h
 *  \brief RPC module for daq monitoring methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brin Dorney <brian.l.dorney@cern.ch>
 */

#ifndef DAQ_MONITOR_H
#define DAQ_MONITOR_H

#include "utils.h"
#include <unistd.h>

const int NOH_MAX = 12;

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

/*! \fn void getmonGBTLinkLocal(const RPCMsg *request, RPCMsg *response);
 *  \brief Local version of getmonGBTLink
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param doReset boolean if true (false) a link reset will (not) be sent
 */
void getmonGBTLinkLocal(localArgs * la, int NOH=12, bool doReset=false);

/*! \fn void getmonGBTLink(const RPCMsg *request, RPCMsg *response);
 *  \brief Reads the GBT link status registers (READY, WAS_NOT_READY, etc...) for a particular ohMask
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonGBTLink(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonOHLinkmain(const RPCMsg *request, RPCMsg *response)
 *  \brief Checks the status of the GBT and VFAT links on this OH in one RPC call
 *  \details Expects the "NOH" RPC key specifying the number of optohybrids.  No local callable version
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHLink(const RPCMsg *request, RPCMsg *response);

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
 *  \brief Reads the SCA Monitoring values of all OH's (voltage and temperature); these quantities are reported in ADC units
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonOHSCAmain(const RPCMsg *request, RPCMsg *response);

/*! \fn void getmonOHSysmonLocal(localArgs *la, int NOH=12, int ohMask=0xfff, bool doReset=false)
 *  \brief Local version of getmonOHSysmon
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param ohMask A 12 bit number which specifies which optohybrids to read from.  Having a value of 1 in the n^th bit indicates that the n^th optohybrid should be considered.
 *  \param doReset reset counters CNT_OVERTEMP, CNT_VCCAUX_ALARM and CNT_VCCINT_ALARM (presently not working in FW)
 */
void getmonOHSysmonLocal(localArgs *la, int NOH=12, int ohMask=0xfff, bool doReset=false);

/*! \fn void getmonOHSysmon(const RPCMsg *request, RPCMsg *response)
 *  \brief reads FPGA Sysmon values of all unmasked OH's
 *  \details Reads FPGA core temperature, core voltage (1V), and I/O voltage (2.5V); these quantities are reported in ADC units.  The LSB for the core temperature correspons to 0.49 C.  The LSB for the core voltage (both 1V and 2.5V) corresponds to 2.93 mV.
 *  \details Will also check error conditions (over temperature, 1V VCCINT, and 2.5V VCCAUX), and the error conunters for those conditions.
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

/*! \fn void getmonVFATLinkLocal(const RPCMsg *request, RPCMsg *response);
 *  \brief Local version of getmonVFATLink
 *  \param la Local arguments
 *  \param NOH Number of optohybrids in FW
 *  \param doReset boolean if true (false) a link reset will (not) be sent
 */
void getmonVFATLinkLocal(localArgs * la, int NOH=12, bool doReset=false);

/*! \fn void getmonVFATLink(const RPCMsg *request, RPCMsg *response);
 *  \brief Reads the VFAT link status registers (LINK_GOOD, SYNC_ERR_CNT, etc...) for a particular ohMask
 *  \param request RPC request message
 *  \param response RPC response message
 */
void getmonVFATLink(const RPCMsg *request, RPCMsg *response);

#endif
