/*! \file amc/daq.h
 *  \brief RPC module for AMC DAQ methods, copied in from cmsgemos/gem::hw::HwGenericAMC
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_DAQ_H
#define AMC_DAQ_H

#include "utils.h"

/*!
 * \defgroup daq DAQ module related setup and status
 */

/** Locally executed methods */
/*!
 * \brief Set the enable mask and enable the DAQ link
 * \param enableMask 32 bit word for the 24 bit enable mask
 */
void enableDAQLinkLocal(localArgs* la, uint32_t const& enableMask=0x1);

/*!
 * \brief Set the DAQ link off and disable all inputs
 */
void disableDAQLinkLocal(localArgs* la);

/*!
 * \brief Set the zero suppression mode
 * \param enable true means any VFAT data packet with all 0's will be suppressed
 */
void setZSLocal(localArgs* la, bool enable=true);

/*!
 * \brief Disable zero suppression of VFAT data
 */
void disableZSLocal(localArgs* la);

/*!
 * \brief reset the DAQ link and write the DAV timout
 *        - assert RESET and then release
 *        - disable DAQ link
 *        - set each link EOE_TIMEOUT to default value
 *        - set DAV_TIMEOUT  to supplied value
 *        - set TTS_OVERRIDE to supplied value
 * \param davTO value to use for the DAV timeout
 * \param ttsOverride value to use for the TTS override
 */
void resetDAQLinkLocal(localArgs* la, uint32_t const& davTO=0x500, uint32_t const& ttsOverride=0x0);

/*!
 * \returns Returns the 32 bit word corresponding to the DAQ link control register
 */
uint32_t getDAQLinkControlLocal(localArgs* la);

/*!
 * \returns Returns the 32 bit word corresponding to the DAQ link status register
 */
uint32_t getDAQLinkStatusLocal(localArgs* la);

/*!
 * \returns Returns true if the DAQ link is ready
 */
bool daqLinkReadyLocal(localArgs* la);

/*!
 * \returns Returns true if the DAQ link is clock is locked
 */
bool daqClockLockedLocal(localArgs* la);

/*!
 * \returns Returns true if the TTC is ready
 */
bool daqTTCReadyLocal(localArgs* la);

/*!
 * \returns Returns the current TTS state asserted by the DAQ link firmware
 */
uint8_t daqTTSStateLocal(localArgs* la);

/*!
 * \returns Returns true if the event FIFO is almost full (70%)
 */
bool daqAlmostFullLocal(localArgs* la);

/*!
 * \returns Returns true if the L1A FIFO is empty (0%)
 */
bool l1aFIFOIsEmptyLocal(localArgs* la);

/*!
 * \returns Returns true if the L1A FIFO is almost full (70%)
 */
bool l1aFIFOIsAlmostFullLocal(localArgs* la);

/*!
 * \returns Returns true if the L1A FIFO is full (100%)
 */
bool l1aFIFOIsFullLocal(localArgs* la);

/*!
 * \returns Returns true if the L1A FIFO is underflos
 */
bool l1aFIFOIsUnderflowLocal(localArgs* la);

/*!
 * \returns Returns the number of events built and sent on the DAQ link
 */
uint32_t getDAQLinkEventsSentLocal(localArgs* la);

/*!
 * \returns Returns the curent L1AID (number of L1As received)
 */
uint32_t getDAQLinkL1AIDLocal(localArgs* la);

/*!
 * \returns Returns the curent L1A rate (in Hz)
 */
uint32_t getDAQLinkL1ARateLocal(localArgs* la);

/*!
 * \returns Returns
 */
uint32_t getDAQLinkDisperErrorsLocal(localArgs* la);

/*!
 * \returns Returns
 */
uint32_t getDAQLinkNonidentifiableErrorsLocal(localArgs* la);

/*!
 * \returns Returns the DAQ link input enable mask
 */
uint32_t getDAQLinkInputMaskLocal(localArgs* la);

/*!
 * \returns Returns the timeout used in the event builder before closing the event and sending the (potentially incomplete) data
 */
uint32_t getDAQLinkDAVTimeoutLocal(localArgs* la);

/*!
 * \param max is a bool specifying whether to query the max timer or the last timer
 * \returns Returns the spent building an event
 */
uint32_t getDAQLinkDAVTimerLocal(localArgs* la, bool const& max);

/*!
 * \param gtx is the input link status to query
 * \returns Returns the the 32-bit word corresponding DAQ status for the specified link
 */
uint32_t getLinkDAQStatusLocal(localArgs* la,    uint8_t const& gtx);

/*!
 * \param gtx is the input link counter to query
 * \param mode specifies whether to query the corrupt VFAT count (0x0) or the event number
 * \returns Returns the link counter for the specified mode
 */
uint32_t getLinkDAQCountersLocal(localArgs* la,  uint8_t const& gtx, uint8_t const& mode);

/*!
 * \param gtx is the input link status to query
 * \returns Returns a block of the last 7 words received from the OH on the link specified
 */
uint32_t getLinkLastDAQBlockLocal(localArgs* la, uint8_t const& gtx);

/*!
 * \returns Returns the timeout before the event builder firmware will close the event and send the data
 */
uint32_t getDAQLinkInputTimeoutLocal(localArgs* la);

/*!
 * \returns Returns the run type stored in the data stream
 */
uint32_t getDAQLinkRunTypeLocal(localArgs* la);

/*!
 * \returns Special run parameters 1,2,3 as a single 24 bit word
 */
uint32_t getDAQLinkRunParametersLocal(localArgs* la);

/*!
 * \returns Special run parameter written into data stream
 */
uint32_t getDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parameter);


/*!
 * \brief Set DAQ link timeout
 * \param inputTO is the number of clock cycles to wait after receipt of last L1A and
 *        last packet received from the optical link before closing an "event"
 *        (in units of 160MHz clock cycles, value/4 for 40MHz clock cycles)
 */
void setDAQLinkInputTimeoutLocal(localArgs* la, uint32_t const& inputTO=0x100);

/*!
 * \brief Special run type to be written into data stream
 * \param rtype is the run type
 */
void setDAQLinkRunTypeLocal(localArgs* la, uint32_t const& rtype);

/*!
 * \param value is a 24 bit word to write into the run paramter portion of the GEM header
 * \returns Set special run parameter to be written into data stream
 */
void setDAQLinkRunParametersLocal(localArgs* la, uint32_t const& value);

/*!
 * \returns Special run parameter written into data stream
 * \param parameter is the number of parameter to be written (1-3)
 * \param value is the run paramter to write into the specified parameter
 */
void setDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parameter, uint8_t const& value);

/** RPC callbacks */
void enableDAQLink(const RPCMsg *request, RPCMsg *response);
void disableDAQLink(const RPCMsg *request, RPCMsg *response);
void setZS(const RPCMsg *request, RPCMsg *response);
void disableZS(const RPCMsg *request, RPCMsg *response);
void resetDAQLink(const RPCMsg *request, RPCMsg *response);
void getDAQLinkControl(const RPCMsg *request, RPCMsg *response);
void getDAQLinkStatus(const RPCMsg *request, RPCMsg *response);
void daqLinkReady(const RPCMsg *request, RPCMsg *response);
void daqClockLocked(const RPCMsg *request, RPCMsg *response);
void daqTTCReady(const RPCMsg *request, RPCMsg *response);
void daqTTSState(const RPCMsg *request, RPCMsg *response);
void daqAlmostFull(const RPCMsg *request, RPCMsg *response);
void l1aFIFOIsEmpty(const RPCMsg *request, RPCMsg *response);
void l1aFIFOIsAlmostFull(const RPCMsg *request, RPCMsg *response);
void l1aFIFOIsFull(const RPCMsg *request, RPCMsg *response);
void l1aFIFOIsUnderflow(const RPCMsg *request, RPCMsg *response);
void getDAQLinkEventsSent(const RPCMsg *request, RPCMsg *response);
void getDAQLinkL1AID(const RPCMsg *request, RPCMsg *response);
void getDAQLinkL1ARate(const RPCMsg *request, RPCMsg *response);
void getDAQLinkDisperErrors(const RPCMsg *request, RPCMsg *response);
void getDAQLinkNonidentifiableErrors(const RPCMsg *request, RPCMsg *response);
void getDAQLinkInputMask(const RPCMsg *request, RPCMsg *response);
void getDAQLinkDAVTimeout(const RPCMsg *request, RPCMsg *response);
void getDAQLinkDAVTimer(const RPCMsg *request, RPCMsg *response);
void getLinkDAQStatus(const RPCMsg *request, RPCMsg *response);
void getLinkDAQCounters(const RPCMsg *request, RPCMsg *response);
void getLinkLastDAQBlock(const RPCMsg *request, RPCMsg *response);
void getDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response);
void getDAQLinkRunType(const RPCMsg *request, RPCMsg *response);
void getDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response);
void getDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response);
void setDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response);
void setDAQLinkRunType(const RPCMsg *request, RPCMsg *response);
void setDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response);
void setDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response);

/** Composite functions, no specific "local" function defined */
void configureDAQModule(const RPCMsg *request, RPCMsg *response);
void enableDAQModule(const RPCMsg *request, RPCMsg *response);

#endif
