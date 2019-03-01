/*! \file src/amc/blaster_ram.cpp
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#include "amc.h"

#include <chrono>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

const int GBT_RAM_SIZE  = 92;    ///< for GE1/1 3 GBTx per OH, 92 32-bit words of configuration per GBT (366 8-bit GBT configurations -> 32-bit words + padding)
const int VFAT_RAM_SIZE = 74;    ///< for GE1/1 24 VFATs per OH, 74 32-bit words of configuration per VFAT (147 16-bit VFAT configurations -> 32-bit words + padding)
const int OH_RAM_SIZE   = 2*100; ///< for GE1/1 100 32-bit words of configuration per OH plus the corresponding OH local address
/*
  sanity checks depending on the size (in 32-bit words) of the compile FW
  GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE
  GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE
  GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE

  // Base address of each block
  GEM_AMC.CONFIG_BLASTER.RAM.GBT
  GEM_AMC.CONFIG_BLASTER.RAM.VFAT
  GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA

  // Pointer to base address of each block
  GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH{0..N}
  GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH{0..N}
  GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH{0..N}
 */

uint32_t getGBTRAMBaseAddr(localArgs *la, uint8_t const& ohN, uint8_t const& gbtN)
{
  std::string regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH%d",int(ohN));
  uint32_t base = getAddress(la, regName);
  base += GBT_RAM_SIZE*gbtN;
  return base;
}

uint32_t getOptoHybridRAMBaseAddr(localArgs *la, uint8_t const& ohN)
{
  std::string regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH%d",int(ohN));
  uint32_t base = getAddress(la, regName);
  base += VFAT_RAM_SIZE*ohN;
  return base;
}

uint32_t getVFATRAMBaseAddr(localArgs *la, uint8_t const& ohN, uint8_t const& vfatN)
{
  std::string regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH%d",int(ohN));
  uint32_t base = getAddress(la, regName);
  return base;
}

bool writeConfRAMLocal(localArgs *la, uint32_t* blob)
{
  // do memory validation on blob
  // strip blob to GBT, OptoHybrid, and VFAT blobs
  uint32_t gbtRAMAddr  = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT"));
  uint32_t ohRAMAddr   = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH"));
  uint32_t vfatRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT"));

  return true;
}

bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob)
{
  LOGGER->log_message(LogManager::DEBUG, "writeGBTConfRAMLocal called");
  // do memory validation on gbtblob
  uint32_t gbtRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT"));

  // send size? or do size validation in writeBlock?
  // write to address, or register name, that can be looked up and type checked
  // writeBlock(gbtRAMAddr, gbtblob, la->response);
  return true;
}

bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob)
{
  LOGGER->log_message(LogManager::DEBUG, "writeOptoHybridConfRAMLocal called");
  // do memory validation on ohblob
  uint32_t ohRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH"));

  // send size? or do size validation in writeBlock?
  // write to address, or register name, that can be looked up and type checked
  // writeBlock(ohRAMAddr, ohblob, la->response);
  return true;
}

bool writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob)
{
  LOGGER->log_message(LogManager::DEBUG, "writeVFATConfRAMLocal called");
  // do memory validation on vfatblob
  uint32_t vfatRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT"));

  // send size? or do size validation in writeBlock?
  // write to address, or register name, that can be looked up and type checked
  // writeBlock(vfatRAMAddr, vfatblob, la->response);
  return true;
}


////////////////// RPC callback methods //////////////////
void writeConfRAM(const RPCMsg *request, RPCMsg *response)
{
  struct localArgs la = getLocalArgs(response);
}

void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response)
{
  struct localArgs la = getLocalArgs(response);
}

void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response)
{
  struct localArgs la = getLocalArgs(response);
}

void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response)
{
  struct localArgs la = getLocalArgs(response);

  uint32_t* vfatblob;
  request->get_word_array("vfatblob", vfatblob);
  bool success = writeVFATConfRAMLocal(&la, vfatblob);
  // LOGGER->log_message(LogManager::INFO, stdsprintf("Determined VFAT Mask for OH%i to be 0x%x",ohN,vfatMask));

  // response->set_word("vfatMask",vfatMask);

  return;
}
