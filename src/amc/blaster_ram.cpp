/*! \file src/amc/blaster_ram.cpp
 *  \author Jared Sturdy <sturdy@cern.ch>
 */

#include "amc/blaster_ram.h"

#include <chrono>
#include <iomanip>
#include <string>
#include <time.h>
#include <thread>
#include <vector>

#include "hw_constants.h"

using namespace utils;

namespace amc {
  namespace blaster {
    auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("logger"));
  }
}

uint32_t amc::blaster::getRAMMaxSize::operator()(BLASTERTypeT const& type) const
{
  uint32_t ram_size = 0x0;
  switch (type) {
  case (BLASTERType::GBT) :
    return utils::readReg("GEM_AMC.CONFIG_BLASTER.STATUS.GBT_RAM_SIZE");
  case (BLASTERType::OptoHybrid) :
    return utils::readReg("GEM_AMC.CONFIG_BLASTER.STATUS.OH_RAM_SIZE");
  case (BLASTERType::VFAT) :
    return utils::readReg("GEM_AMC.CONFIG_BLASTER.STATUS.VFAT_RAM_SIZE");
  case (BLASTERType::ALL) :
    ram_size  = getRAMMaxSize{}(BLASTERType::GBT);
    ram_size += getRAMMaxSize{}(BLASTERType::OptoHybrid);
    ram_size += getRAMMaxSize{}(BLASTERType::VFAT);
    return ram_size;
  default:
    break;
  }

  std::stringstream errmsg;
  errmsg << "Invalid BLASTER type " << type << " specified";
  LOG4CPLUS_ERROR(logger, errmsg.str());

  // FIXME throw? or return 0?
  throw std::range_error(errmsg.str());
  // return ram_size;
}

bool amc::blaster::checkBLOBSize(BLASTERTypeT const& type, size_t const& sz)
{
  uint32_t ram_sz = getRAMMaxSize{}(type);
  bool valid = (sz == ram_sz) ? true : false;
  return valid;
}

uint32_t amc::blaster::getRAMBaseAddr(BLASTERTypeT const& type, uint8_t const& ohN, uint8_t const& partN)
{
  uint32_t base = 0x0;
  std::string regName;
  switch (type) {
  case (BLASTERType::GBT) :
    if (partN > (gbt::GBTS_PER_OH-1)) {
      std::stringstream errmsg;
      errmsg << "Invalid GBT specified: GBT" << partN << " > " << (gbt::GBTS_PER_OH-1);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      throw std::range_error(errmsg.str());
    }
    regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH%d",int(ohN));
    base = getAddress(regName);
    base += gbt::GBT_SINGLE_RAM_SIZE*partN;
    return base;
  case (BLASTERType::OptoHybrid) :
    regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH%d",int(ohN));
    base = getAddress(regName);
    return base;
  case (BLASTERType::VFAT) :
    if (partN > (oh::VFATS_PER_OH-1)) {
      std::stringstream errmsg;
      errmsg << "Invalid GBT specified: VFAT" << partN << " > " << (oh::VFATS_PER_OH-1);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      throw std::range_error(errmsg.str());
    }
    regName = stdsprintf("GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH%d",int(ohN));
    base = getAddress(regName);
    base += vfat::VFAT_SINGLE_RAM_SIZE*partN;
    return base;
  default:
    break;
  }

  std::stringstream errmsg;
  errmsg << "Invalid BLASTER type " << type << " specified";
  LOG4CPLUS_ERROR(logger, errmsg.str());
  throw std::runtime_error(errmsg.str());
}

std::vector<uint32_t> amc::blaster::readConfRAM::operator()(BLASTERTypeT const& type, size_t const& blob_sz) const
{
  uint32_t nwords = 0x0;
  // uint32_t offset = 0x0;

  if (!checkBLOBSize(type, blob_sz)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  std::vector<uint32_t> blob(blob_sz,0);
  std::vector<uint32_t> tmpblob;

  // do basic memory validation on blob
  if (!blob.data()) {
    std::stringstream errmsg;
    errmsg << "Invalid BLOB " << std::hex << std::setw(8) << std::setfill('0') << blob.data()
           << std::dec << " provided to write BLASTER RAM";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  std::stringstream regName;
  regName << "GEM_AMC.CONFIG_BLASTER.RAM";

  LOG4CPLUS_DEBUG(logger, stdsprintf("readConfRAM with type: 0x%x, size: 0x%x", type, blob_sz));
  switch (type) {
  case (BLASTERType::GBT):
    // return readGBTConfRAM{}(blob, getRAMMaxSize{}(BLASTERType::GBT)); // FIXME function does not exist
    regName << ".GBT";
    break;
  case (BLASTERType::OptoHybrid):
    // return readOptoHybridConfRAM{}(blob, getRAMMaxSize{}(BLASTERType::OptoHybrid)); // FIXME function does not exist
    regName << ".OH_FPGA";
    break;
  case (BLASTERType::VFAT):
    // return readVFATConfRAM{}(blob, getRAMMaxSize{}(BLASTERType::VFAT)); // FIXME function does not exist
    regName << ".VFAT";
    break;
  case (BLASTERType::ALL):
    // nwords  = readConfRAM{}(BLASTERType::GBT, blob.data()+nwords, getRAMMaxSize{}(BLASTERType::GBT));
    tmpblob = readConfRAM{}(BLASTERType::GBT, getRAMMaxSize{}(BLASTERType::GBT));
    // offset += getRAMMaxSize{}(BLASTERType::GBT);
    // nwords += readConfRAM{}(BLASTERType::OptoHybrid, blob.data()+nwords, getRAMMaxSize{}(BLASTERType::OptoHybrid));
    blob.insert(blob.end(), tmpblob.begin(), tmpblob.end());
    tmpblob = readConfRAM{}(BLASTERType::OptoHybrid, getRAMMaxSize{}(BLASTERType::OptoHybrid));
    // offset += getRAMMaxSize{}(BLASTERType::OptoHybrid);
    // nwords += readConfRAM{}(BLASTERType::VFAT, blob.data()+nwords, getRAMMaxSize{}(BLASTERType::VFAT));
    blob.insert(blob.end(), tmpblob.begin(), tmpblob.end());
    tmpblob = readConfRAM{}(BLASTERType::VFAT, getRAMMaxSize{}(BLASTERType::VFAT));
    blob.insert(blob.end(), tmpblob.begin(), tmpblob.end());
    return blob;
  default:
    std::stringstream errmsg;
    errmsg << "Invalid BLASTER RAM type "
           << std::hex << std::setw(8) << std::setfill('0') << type << std::dec
           << " selected for read.";
    LOG4CPLUS_ERROR(logger, errmsg.str());

    throw std::range_error(errmsg.str());
  }

  nwords = readBlock(regName.str(), blob.data(), blob_sz);
  LOG4CPLUS_DEBUG(logger, stdsprintf("read: %d words from %s", nwords, regName.str().c_str()));

  return blob;
}

// uint32_t amc::blaster::readGBTConfRAM::operator()(void* gbtblob, size_t const& blob_sz, uint16_t const& ohMask) const
std::vector<uint32_t> amc::blaster::readGBTConfRAM::operator()(uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "readGBTConfRAM called");

  // if (blob_sz > getRAMMaxSize{}(BLASTERType::GBT)) {
  //   std::stringstream errmsg;
  //   errmsg << "Invalid size " << blob_sz << " for GBT BLASTER RAM BLOB";
  //   LOG4CPLUS_ERROR(logger, errmsg.str());
  //   throw std::range_error(errmsg.str());
  // }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    uint32_t blob_sz = RAM_SIZE*NUM_OF_OH;
    std::vector<uint32_t> gbtblob(,0)
    // read full blob to VFAT RAM
    return readBlock("GEM_AMC.CONFIG_BLASTER.RAM.GBT", static_cast<uint32_t*>(gbtblob), blob_sz);
  } else {
    // read blob to specific GBT RAM, as specified by ohMask, support non consecutive OptoHybrids?
    const uint32_t perblk = gbt::GBT_SINGLE_RAM_SIZE*gbt::GBTS_PER_OH;
    uint32_t nwords = 0x0;
    uint32_t* blob = static_cast<uint32_t*>(gbtblob);
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH" << oh;
        nwords += readBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
    return nwords;
  }
}

uint32_t amc::blaster::readOptoHybridConfRAM::operator()(uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "readOptoHybridConfRAM called");

  if (blob_sz > getRAMMaxSize{}(BLASTERType::OptoHybrid)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for OptoHybrid BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::range_error(errmsg.str());
  }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    // read to all OptoHybrids
    return readBlock("GEM_AMC.CONFIG_BLASTER.RAM.OH", ohblob, blob_sz);
  } else {
    // read blob to specific OptoHybrid RAM, as specified by ohMask
    uint32_t nwords = 0x0;
    uint32_t* blob = ohblob;
    const uint32_t perblk = oh::OH_SINGLE_RAM_SIZE;
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH" << oh;
        nwords += readBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
    return nwords;
  }
}

uint32_t amc::blaster::readVFATConfRAM::operator()(uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "readVFATConfRAM called");

  if (blob_sz > getRAMMaxSize{}(BLASTERType::VFAT)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for VFAT BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::range_error(errmsg.str());
  }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    // read full blob to VFAT RAM
    return readBlock("GEM_AMC.CONFIG_BLASTER.RAM.VFAT", vfatblob, blob_sz);
  } else {
    // read `vfatblob` to OH specific VFAT RAM, as specified by ohMask
    uint32_t nwords = 0x0;
    uint32_t* blob = vfatblob;
    const uint32_t perblk = vfat::VFAT_SINGLE_RAM_SIZE*oh::VFATS_PER_OH;
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH" << oh;
        nwords += readBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
    return nwords;
  }
}

void amc::blaster::writeConfRAM::operator()(BLASTERTypeT const& type, std::vector<uint32_t> blob) const
{
  if (!checkBLOBSize(type, blob.size())) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob.size() << " for BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  // do memory validation on blob, doesn't make sense now that this is a vector...
  if (!blob.data()) {
    std::stringstream errmsg;
    errmsg << "Invalid BLOB " << std::hex << std::setw(8) << std::setfill('0') << blob.data()
           << std::dec << " provided to write BLASTER RAM";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }

  auto iter = blob.start();
  std::vector<uint32_t> tmpblob = blob;
  
  LOG4CPLUS_WARN(logger, stdsprintf("writeConfRAM with type: 0x%x, size: 0x%x", type, blob.size()));
  switch (type) {
  case (BLASTERType::GBT):
    writeGBTConfRAM{}(blob.data(), getRAMMaxSize{}(BLASTERType::GBT));
    return;
  case (BLASTERType::OptoHybrid):
    writeOptoHybridConfRAM{}(blob.data(), getRAMMaxSize{}(BLASTERType::OptoHybrid));
    return;
  case (BLASTERType::VFAT):
    writeVFATConfRAM{}(blob.data(), getRAMMaxSize{}(BLASTERType::VFAT));
    return;
  case (BLASTERType::ALL):
    LOG4CPLUS_WARN(logger, "Reading the full RAM");
    tmpblob.clear();
    tmpblob.reserve(getRAMMaxSize{}(BLASTERType::GBT));
    tmpblob.insert(tmpblob.begin(), iter, iter+getRAMMaxSize{}(BLASTERType::GBT));
    writeConfRAM{}(BLASTERType::GBT, tmpblob);

    iter += getRAMMaxSize{}(BLASTERType::GBT);
    tmpblob.clear();
    tmpblob.reserve(getRAMMaxSize{}(BLASTERType::OptoHybrid));
    tmpblob.insert(tmpblob.begin(), iter, iter+getRAMMaxSize{}(BLASTERType::OptoHybrid));
    writeConfRAM{}(BLASTERType::OptoHybrid, tmpblob);

    iter += getRAMMaxSize{}(BLASTERType::OptoHybrid);
    tmpblob.clear();
    tmpblob.reserve(getRAMMaxSize{}(BLASTERType::VFAT));
    tmpblob.insert(tmpblob.begin(), iter, iter+getRAMMaxSize{}(BLASTERType::VFAT));
    writeConfRAM{}(BLASTERType::VFAT, tmpblob);
    return;
  default:
    // writeConfRAM{}(BLASTERType::ALL, blob, blob.size());
    std::stringstream errmsg;
    errmsg << "Invalid BLASTER RAM type "
           << std::hex << std::setw(8) << std::setfill('0') << type << std::dec
           << " selected for write.";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::runtime_error(errmsg.str());
  }
}

void amc::blaster::writeGBTConfRAM::operator()(uint32_t* gbtblob, size_t const& blob_sz, uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "writeGBTConfRAM called");

  if (blob_sz > getRAMMaxSize{}(BLASTERType::GBT)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for GBT BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::range_error(errmsg.str());
  }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    writeBlock("GEM_AMC.CONFIG_BLASTER.RAM.GBT", gbtblob, blob_sz);
  } else {
    // write blob to specific GBT RAM, as specified by ohMask, support non consecutive OptoHybrids?
    const uint32_t perblk = gbt::GBT_SINGLE_RAM_SIZE*gbt::GBTS_PER_OH;
    uint32_t* blob = gbtblob;
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.GBT_OH" << oh;
        writeBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
  }

  return;
}

void amc::blaster::writeOptoHybridConfRAM::operator()(uint32_t* ohblob, size_t const& blob_sz, uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "writeOptoHybridConfRAM called");

  if (blob_sz > getRAMMaxSize{}(BLASTERType::OptoHybrid)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for OptoHybrid BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::range_error(errmsg.str());
  }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    // write to all OptoHybrids
    writeBlock("GEM_AMC.CONFIG_BLASTER.RAM.OH", ohblob, blob_sz);
  } else {
    // write blob to specific OptoHybrid RAM, as specified by ohMask
    uint32_t* blob = ohblob;
    const uint32_t perblk = oh::OH_SINGLE_RAM_SIZE;
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.OH_FPGA_OH" << oh;
        writeBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
  }

  return;
}

void amc::blaster::writeVFATConfRAM::operator()(uint32_t* vfatblob, size_t const& blob_sz, uint16_t const& ohMask) const
{
  LOG4CPLUS_DEBUG(logger, "writeVFATConfRAM called");

  if (blob_sz > getRAMMaxSize{}(BLASTERType::VFAT)) {
    std::stringstream errmsg;
    errmsg << "Invalid size " << blob_sz << " for VFAT BLASTER RAM BLOB";
    LOG4CPLUS_ERROR(logger, errmsg.str());
    throw std::range_error(errmsg.str());
  }

  if (ohMask == 0x0 || ohMask == 0xfff) {
    // write full blob to VFAT RAM
    writeBlock("GEM_AMC.CONFIG_BLASTER.RAM.VFAT", vfatblob, blob_sz);
  } else {
    // write `vfatblob` to OH specific VFAT RAM, as specified by ohMask
    uint32_t* blob = vfatblob;
    const uint32_t perblk = vfat::VFAT_SINGLE_RAM_SIZE*oh::VFATS_PER_OH;
    for (size_t oh = 0; oh < amc::OH_PER_AMC; ++oh) {
      if ((0x1<<oh)&ohMask) {
        std::stringstream reg;
        reg << "GEM_AMC.CONFIG_BLASTER.RAM.VFAT_OH" << oh;
        writeBlock(reg.str(), blob, perblk);
        blob += perblk;
      }
    }
  }

  return;
}
