/*!
 * \file amc/sca.h
 * \brief RPC module for AMC SCA methods, copied in from cmsgemos/gem::hw::HwGenericAMC
 * \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_SCA_H
#define AMC_SCA_H

#include "utils.h"
#include "amc/sca_enums.h"

/*!
 * \defgroup sca SCA module functionality
 */

/*!
 * \brief Prepare data for use with the SCA communication interfaces
 *
 * \details SCA TX/RX data is transmitted using the HDLC protocol, which is 16-bit length, and sent LSB to MSB.
 * In the HDLC packet, it is sent/received as [<16:31><0:15>].
 * The GEM_AMC firmware stores it as [<7:0><15:8><23:16><31:24>]
 *
 * \param data is the data to be converted to the appropriate ordering
 */
uint32_t formatSCAData(uint32_t const& data);

/*!
 * \brief Execute a command using the SCA interface
 * \details Generic command to drive commands to all SCA modules
 *  *
 * \param la Local arguments structure
 * \param ch channel to communicate with
 * \param cmd command to send, referenced in sca_enums.h
 * \param len length of the data to send, available 1,2,4
 * \param data to send to the CTRL command
 * \param ohMask bit list of OptoHybrids to send the commands to
 */
void sendSCACommand(localArgs* la, uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask=0xfff);

/*!
 * \brief Execute a command using the SCA interface, and read the reply from the SCA
 * \param la Local arguments structure
 * \param ch channel to communicate with
 * \param cmd command to send, referenced in sca_enums.h
 * \param len length of the data to send, available 1,2,4
 * \param data to send to the CTRL command
 * \param ohMask bit list of OptoHybrids to send the commands to
 */
std::vector<uint32_t> sendSCACommandWithReply(localArgs* la, uint8_t const& ch, uint8_t const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask=0xfff);

/*!
 * \brief Execute a command using the SCA CTRL interface
 * \details CTRL contains
 *  * generic SCA R/W control registers
 *  * Chip ID (on channel 0x14, ADC)
 *  * SEU counter and reset (on channel 0x13, JTAG)
 * \param la Local arguments structure
 * \param cmd which command to send, referenced in sca_enums.h
 * \param ohMask bit list of OptoHybrids to send the commands to
 * \param len length of the data to send, default is 0x1
 * \param data to send to the CTRL command, default is 0x0
 */
std::vector<uint32_t> scaCTRLCommand(localArgs* la, SCACTRLCommandT const& cmd, uint16_t const& ohMask=0xfff, uint8_t const& len=0x1, uint32_t const& data=0x0);

/** Locally executed methods */
/*!
 * \brief Execute a command using the SCA I2C interface
 * \details I2C bus will be enabled
 *  *
 * \param la Local arguments structure
 * \param ch I2C channel to communicate with
 * \param cmd which command to send, referenced in sca_enums.h
 * \param len length of the data to send, available 1,2,4
 * \param data to send to the I2C command
 * \param ohMask bit list of OptoHybrids to send the commands to
 */
std::vector<uint32_t> scaI2CCommand(localArgs* la, SCAI2CChannelT const& ch, SCAI2CCommandT const& cmd, uint8_t const& len, uint32_t data, uint16_t const& ohMask=0xfff);

/*!
 * \brief Execute a command using the SCA GPIO interface
 * \details GPIO bus will be enabled
 *  *
 * \param la Local arguments structure
 * \param ch GPIO channel to communicate with
 * \param cmd which command to send, referenced in sca_enums.h
 * \param len length of the data to send, available 1,2,4
 * \param data to send to the GPIO command
 * \param ohMask bit list of OptoHybrids to send the commands to
 */
std::vector<uint32_t> scaGPIOCommand(localArgs* la, SCAGPIOChannelT const& ch, uint8_t const& len, uint32_t data, uint16_t const& ohMask=0xfff);

/*!
 * \brief Execute a command using the SCA ADC interface
 *
 * \param la Local arguments structure
 * \param ch ADC channel to communicate with
 * \param len length of the data to send, available 1,2,4
 * \param data to send to the ADC command
 * \param ohMask bit list of OptoHybrids to send the commands to
 */
std::vector<uint32_t> scaADCCommand(localArgs* la, SCAADCChannelT const& ch, uint8_t const& len, uint32_t data, uint16_t const& ohMask=0xfff);

/*** CTRL submodule ***/
/*!
 * \brief Reset the SCA module
 */
void scaModuleResetLocal(localArgs* la, uint16_t const& ohMask);

/*!
 * \brief Reset the SCA module
 *
 * \param en true to enable the TTC HardReset action
 */
void scaHardResetEnableLocal(localArgs* la, bool en);

/*!
 * \brief Read the Chip ID from the SCA ASIC
 *
 * \param la Local arguments structure
 * \param ohMask bit list of OptoHybrids to send the commands to
 * \param scaV1 true for V1 of the SCA ASIC
 */
std::vector<uint32_t> readSCAChipIDLocal(localArgs* la, uint16_t const& ohMask=0xfff, bool scaV1=false);

/*!
 * \brief Read the SEU coutner from the SCA ASIC
 *
 * \param la Local arguments structure
 * \param ohMask bit list of OptoHybrids to send the commands to
 * \param reset true to reset the counter before reading
 */
std::vector<uint32_t> readSCASEUCounterLocal(localArgs* la, uint16_t const& ohMask=0xfff, bool reset=false);

/*!
 * \brief Reset the SCA SEU counter
 *
 * \param la Local arguments structure
 * \param ohMask bit list of OptoHybrids to send the reset command to
 */
void resetSCASEUCounterLocal(localArgs* la, uint16_t const& ohMask=0xfff);

/* /\*** SCA ADC submodule ***\/ */
/* /\*! */
/*  * \brief Reset the SCA module */
/*  *\/ */
/* void scaModuleResetLocal(localArgs* la); */

/* /\*** GPIO CTRL submodule ***\/ */
/* /\*! */
/*  * \brief Reset the SCA module */
/*  *\/ */
/* void scaModuleResetLocal(localArgs* la); */

/* /\*** I2C CTRL submodule ***\/ */
/* /\*! */
/*  * \brief Reset the SCA module */
/*  *\/ */
/* void scaModuleResetLocal(localArgs* la); */

/*** JTAG CTRL submodule ***/
/*!
 * \brief Low priority
 */

/** RPC callbacks */
/*!
 *  \brief RPC method callbacks contain two parameters
 *  \param request RPC request message
 *  \param response RPC response message
 */
void scaModuleReset(const RPCMsg *request, RPCMsg *response);
void readSCAChipID(const RPCMsg *request, RPCMsg *response);
void readSCASEUCounter(const RPCMsg *request, RPCMsg *response);
void resetSCASEUCounter(const RPCMsg *request, RPCMsg *response);

/*!
 *  \fn void readSCAADCSensor(const RPCMsg *request, RPCMsg *response)
 *  \brief Read individual SCA ADC sensor
 *  \param[in] request RPC response message with the following keys:
 *  	- `word ohMask` : This specifies which OH's to read from
 *  	- `word ch` : Name of SCA ADC sensor to read
 *
 *  \param[out] response RPC response message with the following keys:
 *  	- `word_array data` : ADC data is returned as an array of 32-bit words formatted as:
 *  			      bits [31:29]: constant 0's
 *  			      bit  [28]   : data present
 *  			      bits [27:24]: link ID
 *  			      bits [23:21]: constant 0's
 *  			      bits [20:16]: ADC channel ID
 *  			      bits [15:12]: constant 0's
 *  			      bits [11:0] : ADC data
 */
void readSCAADCSensor(const RPCMsg *request, RPCMsg *response);

/*!
 *  \fn void readSCAADCTemperatureSensors(const RPCMsg *request, RPCMsg *response)
 *  \brief Read all SCA ADC temperature sensors. They are 0x00, 0x04, 0x07, and 0x08.
 *  \param[in] request RPC response message with the following keys:
 *  	- `word ohMask` : This specifies which OH's to read from
 *
 *  \param[out] response RPC response message with the following keys:
 *  	- `word_array data` : ADC data is returned as an array of 32-bit words formatted as:
 *  			      bits [31:29]: constant 0's
 *  			      bit  [28]   : data present
 *  			      bits [27:24]: link ID
 *  			      bits [23:21]: constant 0's
 *  			      bits [20:16]: ADC channel ID
 *  			      bits [15:12]: constant 0's
 *  			      bits [11:0] : ADC data
 */
void readSCAADCTemperatureSensors(const RPCMsg *request, RPCMsg *response);

/*!
 *  \fn void readSCAADCVoltageSensors(const RPCMsg *request, RPCMsg *response)
 *  \brief Read all SCA ADC voltages sensors. They are 1B, 1E, 11, 0E, 18 and 0F.
 *  \param[in] request RPC response message with the following keys:
 *  	- `word ohMask` : This specifies which OH's to read from
 *
 *  \param[out] response RPC response message with the following keys:
 *  	- `word_array data` : ADC data is returned as an array of 32-bit words formatted as:
 *  			      bits [31:29]: constant 0's
 *  			      bit  [28]   : data present
 *  			      bits [27:24]: link ID
 *  			      bits [23:21]: constant 0's
 *  			      bits [20:16]: ADC channel ID
 *  			      bits [15:12]: constant 0's
 *  			      bits [11:0] : ADC data
 */
void readSCAADCVoltageSensors(const RPCMsg *request, RPCMsg *response);

/*!
 *  \fn void readSCAADCSignalStrengthSensors(const RPCMsg *request, RPCMsg *response)
 *  \brief Read the SCA ADC signal strength sensors. They are 15, 13 and 12.
 *  \param[in] request RPC response message with the following keys:
 *  	- `word ohMask` : This specifies which OH's to read from
 *
 *  \param[out] response RPC response message with the following keys:
 *  	- `word_array data` : ADC data is returned as an array of 32-bit words formatted as:
 *  			      bits [31:29]: constant 0's
 *  			      bit  [28]   : data present
 *  			      bits [27:24]: link ID
 *  			      bits [23:21]: constant 0's
 *  			      bits [20:16]: ADC channel ID
 *  			      bits [15:12]: constant 0's
 *  			      bits [11:0] : ADC data
 */
void readSCAADCSignalStrengthSensors(const RPCMsg *request, RPCMsg *response);

/*!
 *  \fn void readAllSCAADCSensors(const RPCMsg *request, RPCMsg *response)
 *  \brief Read all connected SCA ADC sensors.
 *  \param[in] request RPC response message with the following keys:
 *  	- `word ohMask` : This specifies which OH's to read from
 *
 *  \param[out] response RPC response message with the following keys:
 *  	- `word_array data` : ADC data is returned as an array of 32-bit words formatted as:
 *  			      bits [31:29]: constant 0's
 *  			      bit  [28]   : data present
 *  			      bits [27:24]: link ID
 *  			      bits [23:21]: constant 0's
 *  			      bits [20:16]: ADC channel ID
 *  			      bits [15:12]: constant 0's
 *  			      bits [11:0] : ADC data
 */
void readAllSCAADCSensors(const RPCMsg *request, RPCMsg *response);

#endif
