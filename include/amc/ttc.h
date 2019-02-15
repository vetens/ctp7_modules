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
 * \details Performs the locking procedure performs up to 3840 shifts (full width of one good + bad region).
 * \details If relock is true, then the procedure will:
 *  * first shift into a bad region (not an edge)
 *  * find the next good lock status
 *  * shift halfway through the region (1920 for BC0_LOCKED, 1000 for PLL_LOCKED)
 * \details If relock is false, then the procedure will find X consecutive "good" locks for X = 200 (50) for BC0_LOCKED (PLL_LOCKED).
 * \details It will then reverse direction and shift backwards halfway.
 * \details If a bad lock is encountered it will reset and try again.  Otherwise it will take the phase at the back half point
 * \param la Local arguments structure
 * \param relock controls whether the procedure will force a relock
 * \param modeBC0 controls whether the procedure will use the BC0_LOCKED or the PLL_LOCKED register.  Note for GEM_AMC FW > 1.13.0 BC0_LOCKED doesn't work.
 * \param scan tells the procedure to run through the full possibility of phases several times, and just logs the place where it found a lock
 */
void ttcMMCMPhaseShiftLocal(localArgs* la, bool relock=false, bool modeBC0=false, bool scan=false);

/*!
 * \brief Resets the MMCM PLL and checks if it relocks
 * \param la Local arguments structure
 * \param readAttempts Specifies the number of times to reset the PLL and check for a relock
 * \return Returns the number of times the MMCM PLL relocked
 */
int checkPLLLockLocal(localArgs* la, int readAttempts);

/*!
 * \brief Get the mean value of the MMCM phase
 * \param readAttempts Specifies the number of times to read the MMCM phase and compute the mean
 *        * 0 means read the mean calculated in the FW
 *        * 1+ means read the mean compute the mean of the specified number of reads
 * \returns Mean value of the MMCH phase
 */
double getMMCMPhaseMeanLocal(localArgs* la, int readAttempts);

/*!
 * \brief Get the mean value of the GTH phase
 * \param readAttempts Specifies the number of times to read the GTH phase and compute the mean
 *        * 0 means read the mean calculated in the FW
 *        * 1+ means read the mean compute the mean of the specified number of reads
 * \returns Mean value (calculated in firmware) of the GTH phase
 */
double getGTHPhaseMeanLocal(localArgs* la, int readAttempts);

/*!
 * \brief Get the median value of the MMCM phase
 * \param readAttempts Specifies the number of times to read the MMCM phase and compute the median
 * \returns Median value of the MMCH phase
 */
double getMMCMPhaseMedianLocal(localArgs* la, int readAttempts);

/*!
 * \brief Get the median value of the GTH phase
 * \param readAttempts Specifies the number of times to read the GTH phase and compute the median
 * \returns Median value of the GTH phase
 */
double getGTHPhaseMedianLocal(localArgs* la, int readAttempts);

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
/*!
 *  \brief RPC method callbacks contain two parameters
 *  \param request RPC request message
 *  \param response RPC response message
 */
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
