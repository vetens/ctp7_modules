/*! \file extras.cpp
 *  \brief Extra register operations methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 */

#include "moduleapi.h"
//#include <libmemsvc.h>
#include "memhub.h"

memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations

/*!
 *  \brief Sequentially reads a block of values from a contiguous address space.
 *  Register mask is not applied
 *
 *  \param register address of the start of the block read
 *  \param number of consecutive words to read from the block
 */
std::vector<uint32_t> mblockread(uint32_t const& addr, uint32_t const& count)
{
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

  std::vector<uint32_t> data(count, 0);

  if (memhub_read(memsvc, addr, count, data.data()) != 0) {
    std::stringstream errmsg;
    errmsg << "blockread memsvc error: " << memsvc_get_last_error(memsvc);
    LOG4CPLUS_ERROR(logger, errmsg.str());
    raise std::runtime_error(errmsg.str); 
  }
  return data;
}

/*!
 *  \brief Sequentially reads a block of values from the same raw register address.
 *  Register mask is not applied
 *  The address acts like a port/FIFO
 *
 *  \param address of FIFO
 *  \param number of words to read from the FIFO
 */
std::vector<uint32_t> mfiforead(uint32_t const& addr, uint32_t const& count)
{
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

  std::vector<uint32_t> data(count, 0);

  for (auto const& val : data) {
    if (memhub_read(memsvc, addr, 1, &val) != 0) {
      std::stringstream errmsg;
      errmsg << "fiforead memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      raise std::runtime_error(errmsg.str); 
    }
  }

  return data;
}

/*!
 *  \brief Reads a list of raw addresses
 *
 *  \param list of addresses to read
 *  \returns list of register values read 
 */
std::vector<uint32_t> mlistread(std::vector<uint32_t> const& reglist)
{
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

  std::vector<uint32_t> data(reglist.size(), 0);

  for (size_t i = 0; i < reglist.size(); ++i) {
    if (memhub_read(memsvc, addr.at(i), 1, &data.at(i)) != 0) {
      std::stringstream errmsg;
      errmsg << "listread memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      raise std::runtime_error(errmsg.str); 
    }
  }

  return data;
}

/*!
 *  \brief writes a block of values to a contiguous memory block
 *
 *  \param address of the start of the block to write
 *  \param values to write into the block
 */
void mblockwrite(uint32_t const& addr, std::vector<uint32_t> const& data)
{
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));

  if (memhub_write(memsvc, addr, count, data.data()) != 0) {
    std::stringstream errmsg;
    errmsg << "blockwrite memsvc error: " << memsvc_get_last_error(memsvc);
    LOG4CPLUS_ERROR(logger, errmsg.str());
    raise std::runtime_error(errmsg.str); 
  }

  return;
}


/*!
 *  \brief writes a set of values to an address that acts as a port or FIFO
 *
 *  \param address of the FIFO
 *  \param data to write to the FIFO
 */
void mfifowrite(uint32_t const& addr, std::vector<uint32_t> const& data)
{
  for (auto const& writeval : data) {
    if (memhub_write(memsvc, addr, 1, &writeval) != 0) {
      std::stringstream errmsg;
      errmsg << "fifowrite memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      raise std::runtime_error(errmsg.str); 
    }
  }
  return;
}

/*!
 *  \brief writes a set of values to a list of addresses
 *
 *  \param unordered map of register address and value to be written
 */
void mlistwrite(std::unordered_map<uint32_t const&, uint32_t const&> regvals)
{
  auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
  for (auto const& regval : regvals){
    if (memhub_write(memsvc, regval.first, 1, &regval.second) != 0) {
      std::stringstream errmsg;
      errmsg << "listwrite memsvc error: " << memsvc_get_last_error(memsvc);
      LOG4CPLUS_ERROR(logger, errmsg.str());
      raise std::runtime_error(errmsg.str); 
    }
  }
  return;
}


extern "C" {
  const char *module_version_key = "extras v1.0.1";
  int module_activity_color = 4;

  void module_init(ModuleManager *modmgr) {
    initLogging();

    if (memhub_open(&memsvc) != 0) {
      auto logger = log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("main"));
      LOG4CPLUS_ERROR(logger, "Unable to connect to memory service: " << 
                      memsvc_get_last_error(memsvc));
      LOG4CPLUS_ERROR(logger, "Unable to load module");
      return;
    }

    xhal::common::rpc::registerMethod<extras::mfiforead>(modmgr);
    xhal::common::rpc::registerMethod<extras::mblockread>(modmgr);
    xhal::common::rpc::registerMethod<extras::mlistread>(modmgr);
    xhal::common::rpc::registerMethod<extras::mfifowrite>(modmgr);
    xhal::common::rpc::registerMethod<extras::mblockwrite>(modmgr);
    xhal::common::rpc::registerMethod<extras::mlistwrite>(modmgr);
  }
}
