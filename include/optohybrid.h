/*! \file vfat3.h
 *  \brief RPC module for optohybrid methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Brin Dorney <brian.l.dorney@cern.ch>
 */
#ifndef OPTOHYBRID_H
#define OPTOHYBRID_H

#include "utils.h"
#include "vfat_parameters.h"
#include <unistd.h>

/*! \fn void broadcastWriteLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_write, uint32_t val_to_write, RPCMsg * response, uint32_t mask = 0xFF000000)
 *  \brief Local callable version of broadcastWrite
 *  \param rtxn LMDB transaction handle
 *  \param dbi LMDB individual database handle
 *  \param oh_number Optohybrid optical link number (string)
 *  \return Bitmask of sync'ed VFATs
 */
void broadcastWriteLocal(localArgs * la, uint32_t ohN, std::string regName, uint32_t value, uint32_t mask = 0xFF000000);

void broadcastWrite(const RPCMsg *request, RPCMsg *response);

void broadcastReadLocal(localArgs * la, uint32_t ohN, std::string regName, uint32_t mask = 0xFF000000);

void broadcastRead(const RPCMsg *request, RPCMsg *response);

// Set default values to VFAT parameters. VFATs will remain in sleep mode
void biasAllVFATsLocal(localArgs * la, uint32_t ohN, uint32_t mask = 0xFF000000);

void setAllVFATsToRunModeLocal(localArgs * la, uint32_t ohN, uint32_t mask = 0xFF000000);

void setAllVFATsToSleepModeLocal(localArgs * la, uint32_t ohN, uint32_t mask = 0xFF000000);

void loadVT1Local(localArgs * la, uint32_t ohN, std::string config_file, uint32_t vt1 = 0x64);

void loadVT1(const RPCMsg *request, RPCMsg *response);

void loadTRIMDACLocal(localArgs * la, uint32_t ohN, std::string config_file);

void loadTRIMDAC(const RPCMsg *request, RPCMsg *response);

void configureVFATs(const RPCMsg *request, RPCMsg *response);

void configureScanModuleLocal(localArgs * la, uint32_t ohN, uint32_t vfatN, uint32_t scanmode, bool useUltra, uint32_t mask, uint32_t ch, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep);

void configureScanModule(const RPCMsg *request, RPCMsg *response);

void printScanConfigurationLocal(localArgs * la, uint32_t ohN, bool useUltra);

void printScanConfiguration(const RPCMsg *request, RPCMsg *response);

void startScanModuleLocal(localArgs * la, uint32_t ohN, bool useUltra);

void startScanModule(const RPCMsg *request, RPCMsg *response);

void getUltraScanResultsLocal(localArgs *la, uint32_t *outData, uint32_t ohN, uint32_t nevts, uint32_t dacMin, uint32_t dacMax, uint32_t dacStep);

void getUltraScanResults(const RPCMsg *request, RPCMsg *response);

void stopCalPulse2AllChannelsLocal(localArgs *la, uint32_t ohN, uint32_t mask, uint32_t ch_min, uint32_t ch_max);

void stopCalPulse2AllChannels(const RPCMsg *request, RPCMsg *response);

void statusOHLocal(localArgs * la, uint32_t ohEnMask);

void statusOH(const RPCMsg *request, RPCMsg *response);

#endif
