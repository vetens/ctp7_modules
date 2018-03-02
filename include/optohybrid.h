#ifndef OPTOHYBRID_H
#define OPTOHYBRID_H

#include "utils.h"
#include "vfat_parameters.h"
#include <unistd.h>

void broadcastWriteLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_write, uint32_t val_to_write, RPCMsg * response, uint32_t mask = 0xFF000000);

void broadcastWrite(const RPCMsg *request, RPCMsg *response);

void broadcastReadLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string reg_to_read, RPCMsg * response, uint32_t mask = 0xFF000000);

void broadcastRead(const RPCMsg *request, RPCMsg *response);

// Set default values to VFAT parameters. VFATs will remain in sleep mode
void biasAllVFATsLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg *response, uint32_t mask = 0xFF000000);

void setAllVFATsToRunModeLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg * response, uint32_t mask = 0xFF000000);

void setAllVFATsToSleepModeLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, RPCMsg * response, uint32_t mask = 0xFF000000);

void loadVT1Local(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string config_file, RPCMsg * response, uint32_t vt1 = 0x64);

void loadVT1(const RPCMsg *request, RPCMsg *response);

void loadTRIMDACLocal(lmdb::txn & rtxn, lmdb::dbi & dbi, std::string oh_number, std::string config_file, RPCMsg * response);

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
