/*! \file utils.h
 *  \brief Util methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef UTILS_H
#define UTILS_H

#include "moduleapi.h"
//#include <libmemsvc.h>
#include "memhub.h"
#include "lmdb_cpp_wrapper.h"
#include "xhal/utils/XHALXMLParser.h"

#include <unistd.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>
#include <cstdio>

extern memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations

/*! \struct localArgs
 *  Contains arguments required to execute the method locally
 */
typedef struct localArgs {
    lmdb::txn & rtxn; /*!< LMDB transaction handle */
    lmdb::dbi & dbi;  /*!< LMDB individual database handle */
    RPCMsg *response; /*!< RPC response message */
} LocalArgs;

/*!
 * \brief returns a set up LocalArgs structure
 */
LocalArgs getLocalArgs(RPCMsg *response);

// FIXME: to be replaced with the above function when the struct is properly implemented
#define GETLOCALARGS(response)                                  \
    auto env = lmdb::env::create();                             \
    env.set_mapsize(1UL * 1024UL * 1024UL * 40UL); /* 40 MiB */ \
    std::string gem_path       = std::getenv("GEM_PATH");       \
    std::string lmdb_data_file = gem_path+"/address_table.mdb"; \
    env.open(lmdb_data_file.c_str(), 0, 0664);                  \
    auto rtxn = lmdb::txn::begin(env, nullptr, MDB_RDONLY);     \
    auto dbi  = lmdb::dbi::open(rtxn, nullptr);                 \
    LocalArgs la = {.rtxn     = rtxn,                           \
                    .dbi      = dbi,                            \
                    .response = response};


template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim);

std::string serialize(xhal::utils::Node n);

/*! \brief This macro is used to terminate a function if an error occurs. It logs the message, write it to the `error` RPC key and returns the `error_code` value.
 *  \param response A pointer to the RPC response object.
 *  \param message The `std::string` error message.
 *  \param error_code Value which is passed to the `return` statement.
 */
#define EMIT_RPC_ERROR(response, message, error_code){ \
    LOGGER->log_message(LogManager::ERROR, message); \
    response->set_string("error", message); \
    return error_code; }

/*! \fn uint32_t getNumNonzeroBits(uint32_t value)
 *  \brief returns the number of nonzero bits in an integer
 *  \param value integer to check the number of nonzero bits
 */
uint32_t getNumNonzeroBits(uint32_t value);

/*! \fn uint32_t getMask(LocalArgs * la, const std::string & regName)
 *  \brief Returns the mask for a given register
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t getMask(LocalArgs * la, const std::string & regName);

/*! \fn void writeRawAddress(uint32_t address, uint32_t value, RPCMsg *response)
 *  \brief Writes a value to a raw register address. Register mask is not applied
 *  \param address Register address
 *  \param value Value to write
 *  \param response RPC response message
 */
void writeRawAddress(uint32_t address, uint32_t value, RPCMsg *response);

/*! \fn uint32_t readRawAddress(uint32_t address, RPCMsg *response)
 *  \brief Reads a value from raw register address. Register mask is not applied
 *  \param address Register address
 *  \param response RPC response message
 */
uint32_t readRawAddress(uint32_t address, RPCMsg* response);

/*! \fn uint32_t getAddress(LocalArgs * la, const std::string & regName)
 *  \brief Returns an address of a given register
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t getAddress(LocalArgs * la, const std::string & regName);

/*! \fn void writeAddress(lmdb::val & db_res, uint32_t value, RPCMsg *response)
 *  \brief Writes given value to the address. Register mask is not applied
 *  \param db_res LMDB call result
 *  \param value Value to write
 *  \param response RPC response message
 */
void writeAddress(lmdb::val & db_res, uint32_t value, RPCMsg *response);

/*! \fn uint32_t readAddress(lmdb::val & db_res, RPCMsg *response)
 *  \brief Reads given value to the address. Register mask is not applied
 *  \param db_res LMDB call result
 *  \param response RPC response message
 */
uint32_t readAddress(lmdb::val & db_res, RPCMsg *response);

/*! \fn void writeRawReg(LocalArgs * la, const std::string & regName, uint32_t value)
 *  \brief Writes a value to a raw register. Register mask is not applied
 *  \param la Local arguments structure
 *  \param regName Register name
 *  \param value Value to write
 */
void writeRawReg(LocalArgs * la, const std::string & regName, uint32_t value);

/*! \fn uint32_t uint32_t readRawReg(LocalArgs * la, const std::string & regName)
 *  \brief Reads a value from raw register. Register mask is not applied
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t readRawReg(LocalArgs * la, const std::string & regName);

/*! \fn uint32_t applyMask(uint32_t data, uint32_t mask)
 *  \brief Returns the data with register mask applied
 *  \param data Register data
 *  \param mask Register mask
 */
uint32_t applyMask(uint32_t data, uint32_t mask);

/*! \fn uint32_t readReg(LocalArgs * la, const std::string & regName)
 *  \brief Reads a value from register. Register mask is applied. Will return 0xdeaddead if register is no accessible
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t readReg(LocalArgs * la, const std::string & regName);

/*! \fn void writeReg(LocalArgs * la, const std::string & regName, uint32_t value)
 *  \brief Writes a value to a register. Register mask is applied
 *  \param la Local arguments structure
 *  \param regName Register name
 *  \param value Value to write
 */
void writeReg(LocalArgs * la, const std::string & regName, uint32_t value);

#endif
