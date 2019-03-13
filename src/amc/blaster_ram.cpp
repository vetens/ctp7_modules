/*! \file src/amc/blaster_ram.cpp
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#include "amc/blaster_ram.h"

#include <chrono>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

#include "amc/blaster_ram_defs.h"

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
  base += OH_RAM_SIZE*ohN;
  return base;
}

uint32_t getVFATRAMBaseAddr(localArgs *la, uint8_t const& ohN, uint8_t const& vfatN)
{
  std::string regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH%d",int(ohN));
  uint32_t base = getAddress(la, regName);
  return base;
}

bool writeConfRAMLocal(localArgs *la, uint32_t* blob, size_t const& blob_sz)
{
  // do memory validation on blob
  // strip blob to GBT, OptoHybrid, and VFAT blobs
  // uint32_t gbtRAMAddr  = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT"));
  // uint32_t ohRAMAddr   = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH"));
  // uint32_t vfatRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT"));

  size_t gbtBase  = 0x0;
  // size_t ohBase   = gbtBase+GBT_RAM_SIZE*N_GBTX*N_OH;
  // size_t vfatBase = ohBase+OH_RAM_SIZE*N_OH;

  uint32_t* gbtblob  = blob+gbtBase;
  uint32_t* ohblob   = gbtblob+GBT_RAM_SIZE*N_GBTX;
  uint32_t* vfatblob = ohblob+OH_RAM_SIZE*N_OH;

  writeGBTConfRAMLocal(la,        gbtblob,  GBT_RAM_SIZE*N_GBTX*N_OH);
  writeOptoHybridConfRAMLocal(la, ohblob,   OH_RAM_SIZE*N_OH        );
  writeVFATConfRAMLocal(la,       vfatblob, VFAT_RAM_SIZE*N_VFAT    );
  
  return true;
}

bool writeGBTConfRAMLocal(localArgs *la, uint32_t* gbtblob, size_t const& blob_sz, uint16_t const& ohMask)
{
  LOGGER->log_message(LogManager::DEBUG, "writeGBTConfRAMLocal called");

  // write full blob to VFAT RAM
  writeBlock(la, "GEM_AMC.CONFIG_BLASTER.RAM.GBT", gbtblob, blob_sz);

  // write blob to specific GBT RAM, as specified by ohMask
  for (size_t oh = 0; oh < 12; ++oh) {
    if ((0x1<<oh)&ohMask) {
      uint32_t* blob = gbtblob+(GBT_RAM_SIZE*N_GBTX*oh);
      std::stringstream reg;
      reg << "GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH" << oh;
      writeBlock(la, reg.str(), blob, GBT_RAM_SIZE);
    }
  }

  // do memory validation on gbtblob
  // uint32_t gbtRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT"));
  // writeBlock(gbtRAMAddr, gbtblob, la->response);
  return true;
}

bool writeOptoHybridConfRAMLocal(localArgs *la, uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask)
{
  LOGGER->log_message(LogManager::DEBUG, "writeOptoHybridConfRAMLocal called");

  // write to all OptoHybrids
  writeBlock(la, "GEM_AMC.CONFIG_BLASTER.RAM.OH", ohblob, blob_sz);

  // write blob to specific OptoHybrid RAM, as specified by ohMask
  for (size_t oh = 0; oh < 12; ++oh) {
    if ((0x1<<oh)&ohMask) {
      uint32_t* blob = ohblob+(OH_RAM_SIZE*oh);
      std::stringstream reg;
      reg << "GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH" << oh;
      writeBlock(la, reg.str(), blob, OH_RAM_SIZE);
    }
  }

  // do memory validation on ohblob
  // uint32_t ohRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH"));
  // writeBlock(ohRAMAddr, ohblob, la->response);
  return true;
}

bool writeVFATConfRAMLocal(localArgs *la, uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask)
{
  LOGGER->log_message(LogManager::DEBUG, "writeVFATConfRAMLocal called");

  // write full blob to VFAT RAM
  writeBlock(la, "GEM_AMC.CONFIG_BLASTER.RAM.VFAT", vfatblob, blob_sz);

  // write blob to specific VFAT RAM, as specified by ohMask
  for (size_t oh = 0; oh < 12; ++oh) {
    if ((0x1<<oh)&ohMask) {
      uint32_t* blob = vfatblob+(VFAT_RAM_SIZE*N_VFAT*oh);
      std::stringstream reg;
      reg << "GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH" << oh;
      writeBlock(la, reg.str(), blob, VFAT_RAM_SIZE);
    }
  }

  // do memory validation on vfatblob
  // uint32_t vfatRAMAddr = getAddress(la, stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT"));
  // writeBlock(vfatRAMAddr, vfatblob, la->response);
  return true;
}


////////////////// RPC callback methods //////////////////
void writeConfRAM(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
}

void writeGBTConfRAM(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t* gbtblob;
  request->get_word_array("gbtblob", gbtblob);
  uint32_t blob_sz =  request->get_word("blob_sz");
  bool success = writeGBTConfRAMLocal(&la, gbtblob, blob_sz);
}

void writeOptoHybridConfRAM(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t* ohblob;
  request->get_word_array("ohblob", ohblob);
  uint32_t blob_sz =  request->get_word("blob_sz");
  bool success = writeOptoHybridConfRAMLocal(&la, ohblob, blob_sz);
}

void writeVFATConfRAM(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);

  uint32_t* vfatblob;
  request->get_word_array("vfatblob", vfatblob);
  uint32_t blob_sz =  request->get_word("blob_sz");
  bool success = writeVFATConfRAMLocal(&la, vfatblob, blob_sz);
  // LOGGER->log_message(LogManager::INFO, stdsprintf("Determined VFAT Mask for OH%i to be 0x%x",ohN,vfatMask));

  // response->set_word("vfatMask",vfatMask);

  return;
}
