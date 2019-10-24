/*! \file utils.h
 *  \brief util methods for RPC modules running on a Zynq
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef UTILS_H
#define UTILS_H

// #include <libmemsvc.h>
#include "moduleapi.h"
#include "memhub.h"

#include "lmdb_cpp_wrapper.h"

#include "xhal/common/utils/XHALXMLParser.h"
#include "xhal/common/rpc/common.h"

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

extern memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations
/* extern log4cplus logger; /// \var global logger */

/*!
 * \brief Environment variable name storing the \c log4cplus configuration filename
 */
constexpr auto LOGGING_CONFIGURATION_ENV = "RPCSVC_LOGGING_CONF";

/*!
 * \brief Default \c log4cplus configuration used when the configuration file cannot be read
 */
constexpr auto LOGGING_DEFAULT_CONFIGURATION = R"----(
log4cplus.rootLogger=INFO,syslog
log4cplus.appender.syslog=log4cplus::SysLogAppender
log4cplus.appender.syslog.ident=rpcsvc
log4cplus.appender.syslog.facility=user
log4cplus.appender.syslog.layout=log4cplus::PatternLayout
log4cplus.appender.syslog.layout.ConversionPattern= %h[%i] - %M - %m
)----";

namespace xhal {
  namespace common {
    namespace rpc {
      template<typename Message>
      inline void serialize(Message &msg, int &value) {
        msg & value;
      }
          
      template<typename Message>
      inline void serialize(Message &msg, bool &value) {
        msg & value;
      }
          
      template<typename Message>
      inline void serialize(Message &msg, uint8_t &value) {
        msg & value;
      }
          
      template<typename Message>
      inline void serialize(Message &msg, uint16_t &value) {
        msg & value;
      }

      template<typename Message>
      inline void serialize(Message &msg, unsigned char &value) {
        msg & value;
      }
          
      template<typename Message>
      inline void serialize(Message &msg, unsigned int &value) {
        msg & value;
      }          

      template<typename Message>
      inline void serialize(Message &msg, double &value) {
        msg & *(reinterpret_cast<uint32_t*>(&value));
      }
          
      template<typename Message>
      inline void serialize(Message &msg, float &value) {
        msg & *(reinterpret_cast<uint32_t*>(&value));
      }

      template<typename Message>
      inline double deserialize(Message &msg, double &value) {
        msg & *(reinterpret_cast<uint32_t*>(&value));
      }
          
      template<typename Message>
      inline float deserialize(Message &msg, float &value) {
        msg & *(reinterpret_cast<uint32_t*>(&value));
      }
    }
  }
}

namespace utils {

    /*!
     *  \struct localArgs
     *  \brief Contains arguments required to execute the method locally
     */
    struct LocalArgs {
        lmdb::txn &rtxn; ///< LMDB transaction handle
        lmdb::dbi &dbi;  ///< LMDB individual database handle
    };

    static std::unique_ptr<LocalArgs> la;

    /*!
     *  \struct RegInfo
     *  \brief Contains information stored in the address table for a given register node
     */
    struct RegInfo {
        std::string permissions; ///< Named register permissions: r,w,rw
        std::string mode;        ///< Named register mode: s(ingle),b(lock)
        uint32_t    address;     ///< Named register address
        uint32_t    mask;        ///< Named register mask
        uint32_t    size;        ///< Named register size, in 32-bit words

        /*!
         * \brief With its intrusive serializer
         */
        template<class Message> void serialize(Message & msg) {
            msg & permissions & mode & address & mask & size;
        }

        friend std::ostream& operator<<(std::ostream &o, RegInfo const& r) {
            o << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.address << std::dec << "  "
              << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.mask    << std::dec << "  "
              << "0x" << std::hex << std::setw(8) << std::setfill('0') << r.size    << std::dec << "  "
              << r.mode << "  " << r.permissions;
            return o;
        }
    };
    
    static constexpr uint32_t LMDB_SIZE = 1UL * 1024UL * 1024UL * 50UL; ///< Maximum size of the LMDB object, currently 50 MiB

  // FIXME: to be removed when the LMDB singleton is properly implemented
#define GETLOCALARGS()                                          \
    auto env = lmdb::env::create();                             \
    env.set_mapsize(utils::LMDB_SIZE);                          \
    std::string gem_path       = std::getenv("GEM_PATH");       \
    std::string lmdb_data_file = gem_path+"/address_table.mdb"; \
    env.open(lmdb_data_file.c_str(), 0, 0664);                  \
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);     \
    auto dbi  = lmdb::dbi::open(rtxn, nullptr);                 \
    utils::LocalArgs la = {.rtxn = rtxn,                        \
                           .dbi  = dbi};

    /*!
     *  \brief Tokenize a string based on a delimieter
     *  \param s The \c std::string object to tokenize
     *  \param delim \c char value to use as tokenizer
     *  \tparam Out \c Out value to use as tokenizer
     */
    template<typename Out>
    void split(const std::string &s, char delim, Out result) {
        std::stringstream ss;
        ss.str(s);
        std::string item;
        while (std::getline(ss, item, delim)) {
            *(result++) = item;
        }
    }

    /*!
     *  \brief Tokenize a string based on a delimieter
     *  \param s The \c std::string object to tokenize
     *  \param delim \c char value to use as tokenizer
     */
    std::vector<std::string> split(const std::string &s, char delim);

    /*!
     *  \brief Serialize an \c xhal::common::utils::Node
     *  \param n The \c xhal::common::utils::Node object to serialize
     */
    std::string serialize(xhal::common::utils::Node n);

    /*!
     * \brief This function initializes the `log4cplus` logging system
     *
     * It first tries to read the \c LOGGING_CONFIGURATION_FILENAME.
     * If the file is not found, it defaults to the embedded configuration LOGGING_DEFAULT_CONFIGURATION.
     */
    void initLogging();

    /*! \fn uint32_t getNumNonzeroBits(uint32_t value)
     *  \brief returns the number of nonzero bits in an integer
     *  \param value integer to check the number of nonzero bits
     */
    uint32_t getNumNonzeroBits(uint32_t value);

    /*!
     *  \brief This macro is used to terminate a function if an error occurs.
     *         It logs the message  and returns the `error_code` value.
     *  \param message The `std::string` error message.
     *  \param error_code Value that is passed to the `return` statement.
     */
#define EMIT_RPC_ERROR(message, error_code) {           \
      LOG4CPLUS_ERROR(logger, message);                 \
      return error_code;                                \
    }

    /*!
     *  \brief returns the number of nonzero bits in an integer
     *  \param value integer to check the number of nonzero bits
     */
    uint32_t getNumNonzeroBits(uint32_t value);

    /*!
     *  \brief Returns the mask for a given register
     *  \param regName Register name
     */
    uint32_t getMask(const std::string &regName);

    /*!
     *  \brief Writes a value to a raw register address. Register mask is not applied
     *  \param address Register address
     *  \param value Value to write
     */
    void writeRawAddress(uint32_t address, uint32_t value);

    /*!
     *  \brief Reads a value from raw register address. Register mask is not applied
     *  \param address Register address
     */
    uint32_t readRawAddress(uint32_t address);

    /*!
     *  \brief Returns an address of a given register
     *  \param regName Register name
     */
    uint32_t getAddress(const std::string &regName);

    /*!
     *  \brief Writes given value to the address. Register mask is not applied
     *  \param db_res LMDB call result
     *  \param value Value to write
     */
    void writeAddress(lmdb::val &db_res, uint32_t value);

    /*!
     *  \brief Reads given value to the address. Register mask is not applied
     *  \param db_res LMDB call result
     */
    uint32_t readAddress(lmdb::val &db_res);

    /*!
     *  \brief Writes a value to a raw register. Register mask is not applied
     *  \param regName Register name
     *  \param value Value to write
     */
    void writeRawReg(const std::string &regName, uint32_t value);

    /*!
     *  \brief Reads a value from raw register. Register mask is not applied
     *  \param regName Register name
     */
    uint32_t readRawReg(const std::string &regName);

    /*!
     *  \brief Returns the data with register mask applied
     *  \param data Register data
     *  \param mask Register mask
     */
    uint32_t applyMask(uint32_t data, uint32_t mask);

    /*!
     *  \brief Reads a value from register. Register mask is applied. Will return 0xdeaddead if register is no accessible
     *  \param regName Register name
     */
    uint32_t readReg(const std::string &regName);

    /*!
     *  \brief Reads a block of values from a contiguous address space.
     *  \param regName Register name of the block to be read
     *  \param size number of words to read (should this just come from the register properties?
     *  \param result Pointer to an array to hold the result
     *  \param offset Start reading from an offset from the base address returned by regName
     *  \returns the number of uint32_t words in the result (or better to return a std::vector?
     */
    uint32_t readBlock(const std::string &regName, uint32_t *result, const uint32_t &size, const uint32_t &offset=0);

    /*!
     *  \brief Reads a block of values from a contiguous address space.
     *
     *  \param regAddr Register address of the block to be read
     *  \param size number of words to read
     *  \param result Pointer to an array to hold the result
     *  \param offset Start reading from an offset from the base address regAddr
     *  \returns the number of uint32_t words in the result (or better to return a std::vector?
     */
    uint32_t readBlock(const uint32_t &regAddr,  uint32_t *result, const uint32_t &size, const uint32_t &offset=0);

    /*!
     *  \brief Writes a value to a register. Register mask is applied
     *  \param regName Register name
     *  \param value Value to write
     */
    void writeReg(const std::string &regName, uint32_t value);

    /*!
     *  \brief Writes a block of values to a contiguous address space.
     *  \detail Block writes are allowed on 'single' registers, provided:
     *           * The size of the data to be written is 1
     *           * The register does not have a mask
     *          Block writes are allowed on 'block' and 'fifo/incremental/port' type addresses provided:
     *          * The size does not overrun the block as defined in the address table
     *          * Including cases where an offset is provided, baseaddr+offset+size > blocksize
     *  \param regName Register name of the block to be written
     *  \param values Values to write to the block
     *  \param offset Start writing at an offset from the base address returned by regName
     */
    void writeBlock(const std::string &regName, const uint32_t *values, const uint32_t &size, const uint32_t &offset=0);

    /*!
     *  \brief Writes a block of values to a contiguous address space.
     *  \detail Block writes are allowed on 'single' registers, provided:
     *           * The size of the data to be written is 1
     *           * The register does not have a mask
     *          Block writes are allowed on 'block' and 'fifo/incremental/port' type addresses provided:
     *           * The size does not overrun the block as defined in the address table
     *           * Including cases where an offset is provided, baseaddr+offset+size > blocksize
     *  \param regAddr Register address of the block to be written
     *  \param values Values to write to the block
     *  \param size number of words to write
     *  \param offset Start writing at an offset from the base address regAddr
     */
    void writeBlock(const uint32_t &regAddr, const uint32_t *values, const uint32_t &size, const uint32_t &offset=0);

    /*!
     *  \brief Updates the LMDB object
     *
     *  \param at_xml is the name of the XML address table to use to populate the LMDB
     */
    struct update_address_table : public xhal::common::rpc::Method
    {
        void operator()(const std::string &at_xml) const;
    };

    /*!
     *  \brief Read register information from LMDB
     *
     *  \param regName Name of node to read from DB
     *  \returns RegInfo object with the properties of the found node
     */
    struct readRegFromDB : public xhal::common::rpc::Method
    {
        RegInfo operator()(const std::string &regName) const;
    };
}
#endif
