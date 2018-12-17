/*!
 * \file amc/ttc.h
 * \brief RPC module for AMC TTC methods, copied in from cmsgemos/gem::hw::HwGenericAMC
 * \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_TTC_H
#define AMC_TTC_H

#include "utils.h"

/*!
 * \defgroup ttc TTC module functionality
 */

/** Locally executed methods */
/*** CTRL submodule ***/
/*!
 * \brief Reset the TTC module
 */
void ttcModuleResetLocal(localArgs* la);

/*!
 * \brief Reset the MMCM of the TTC module
 */
void ttcMMCMResetLocal(localArgs* la);

/*!
 * \brief Shift the phase of the MMCM of the TTC module
 * \param shiftOutOfLockFirst to shift of lock before looking for a good lock
 * \param useBC0Locked to determine the good phase region, rather than the PLL lock status
 * \param doScan whether to roll around multiple times for monitoring purposes
 */
void ttcMMCMPhaseShiftLocal(localArgs* la, bool shiftOutOfLockFirst=false, bool useBC0Locked=false, bool doScan=false);

/*!
 * \brief Check the lock status of the MMCM PLL
 * \param Number of times to read the PLL lock status
 * \returns Lock count of the MMCM PLL
 */
int checkPLLLockLocal(localArgs* la, int readAttempts);

/*!
 * \brief Get the mean value of the MMCM phase
 * \returns Mean value (calculated in firmware) of the MMCH phase
 */
uint32_t getMMCMPhaseMeanLocal(localArgs* la);

/*!
 * \brief Get the mean value of the GTH phase
 * \returns Mean value (calculated in firmware) of the GTH phase
 */
uint32_t getGTHPhaseMeanLocal(localArgs* la);

/*!
 * \brief Get the median value of the MMCM phase
 * \returns Median value of the MMCH phase
 */
uint32_t getMMCMPhaseMedianLocal(localArgs* la);

/*!
 * \brief Get the median value of the GTH phase
 * \returns Median value of the GTH phase
 */
uint32_t getGTHPhaseMedianLocal(localArgs* la);

/*!
 * \brief Reset the counters of the TTC module
 */
void ttcCounterResetLocal(localArgs* la);

/*!
 * \returns whether or not L1As are currently enabled on the GenericAMC
 */
bool getL1AEnableLocal(localArgs* la);

/*!
 * \param whether or not to enable L1As on the GenericAMC
 */
void setL1AEnableLocal(localArgs* la, bool enable=true);

/*** CONFIG submodule ***/
/*!
 * \param cmd AMCTTCCommandT enum type to retrieve the current configuration of
 * \returns TTC configuration register values
 */
uint32_t getTTCConfigLocal(localArgs* la, uint8_t const& cmd);

/*!
 * \param cmd AMCTTCCommandT to set the current configuration of
 */
void setTTCConfigLocal(localArgs* la, uint8_t const& cmd, uint8_t const& value);

/*** STATUS submodule ***/
/*!
 * \brief Returns the first status register of the TTC module
 */
uint32_t getTTCStatusLocal(localArgs* la);

/*!
 * \brief Returns the error count of the TTC module
 * \param specify whether single or double error count
 */
uint32_t getTTCErrorCountLocal(localArgs* la, bool const& single=true);

/*** CMD_COUNTERS submodule ***/
/*!
 * \param cmd to get the current configuration of
 * \returns Returns the counter for the specified TTC command
 */
uint32_t getTTCCounterLocal(localArgs* la, uint8_t const& cmd);

/*!
 * \returns Returns the L1A ID received by the TTC module
 */
uint32_t getL1AIDLocal(localArgs* la);

/*!
 * \returns Returns the curent L1A rate (in Hz)
 */
uint32_t getL1ARateLocal(localArgs* la);

/*!
 * \brief Query the entries in the TTC spy buffer
 * \returns 32-bit word corresponding to the 8 most recent TTC commands received
 */
uint32_t getTTCSpyBufferLocal(localArgs* la);

/** RPC callbacks */
void ttcModuleReset(    const RPCMsg *request, RPCMsg *response);
void ttcMMCMReset(      const RPCMsg *request, RPCMsg *response);
void ttcMMCMPhaseShift( const RPCMsg *request, RPCMsg *response);
void checkPLLLock(      const RPCMsg *request, RPCMsg *response);
void getMMCMPhaseMean(  const RPCMsg *request, RPCMsg *response);
void getMMCMPhaseMedian(const RPCMsg *request, RPCMsg *response);
void getGTHPhaseMean(   const RPCMsg *request, RPCMsg *response);
void getGTHPhaseMedian( const RPCMsg *request, RPCMsg *response);
void ttcCounterReset(   const RPCMsg *request, RPCMsg *response);
void getL1AEnable(      const RPCMsg *request, RPCMsg *response);
void setL1AEnable(      const RPCMsg *request, RPCMsg *response);
void getTTCConfig(      const RPCMsg *request, RPCMsg *response);
void setTTCConfig(      const RPCMsg *request, RPCMsg *response);
void getTTCStatus(      const RPCMsg *request, RPCMsg *response);
void getTTCErrorCount(  const RPCMsg *request, RPCMsg *response);
void getTTCCounter(     const RPCMsg *request, RPCMsg *response);
void getL1AID(          const RPCMsg *request, RPCMsg *response);
void getL1ARate(        const RPCMsg *request, RPCMsg *response);
void getTTCSpyBuffer(   const RPCMsg *request, RPCMsg *response);

#endif
