/*! \file utils.h
 *  \brief Util methods for RPC modules
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 */

#ifndef UTILS_H
#define UTILS_H

#include "moduleapi.h"
#include <libmemsvc.h>
#include "lmdb_cpp_wrapper.h"
#include "xhal/utils/XHALXMLParser.h"
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <vector>
#include <iterator>
#include <cstdio>

memsvc_handle_t memsvc; /// \var global memory service handle required for registers read/write operations

/*! \struct localArgs
 *  Contains arguments required to execute the method locally
 */
struct localArgs {
    lmdb::txn & rtxn; /*!< LMDB transaction handle */
    lmdb::dbi & dbi; /*!< LMDB individual database handle */
    RPCMsg *response; /*!< RPC response message */
};


template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}
std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

std::string serialize(xhal::utils::Node n) {
  return std::to_string((uint32_t)n.real_address)+"|"+n.permission+"|"+std::to_string((uint32_t)n.mask);
}

/*! \fn uint32_t getNumNonzeroBits(uint32_t value)
 *  \brief returns the number of nonzero bits in an integer
 *  \param value integer to check the number of nonzero bits
 */
uint32_t getNumNonzeroBits(uint32_t value);

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

/*! \fn uint32_t getAddress(localArgs * la, const std::string & regName)
 *  \brief Returns an address of a given register
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t getAddress(localArgs * la, const std::string & regName);

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

/*! \fn void writeRawReg(localArgs * la, const std::string & regName, uint32_t value)
 *  \brief Writes a value to a raw register. Register mask is not applied
 *  \param la Local arguments structure
 *  \param regName Register name
 *  \param value Value to write
 */
void writeRawReg(localArgs * la, const std::string & regName, uint32_t value);

/*! \fn uint32_t uint32_t readRawReg(localArgs * la, const std::string & regName)
 *  \brief Reads a value from raw register. Register mask is not applied
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t readRawReg(localArgs * la, const std::string & regName);

/*! \fn uint32_t applyMask(uint32_t data, uint32_t mask)
 *  \brief Returns the data with register mask applied
 *  \param data Register data
 *  \param mask Register mask
 */
uint32_t applyMask(uint32_t data, uint32_t mask);

/*! \fn uint32_t readReg(localArgs * la, const std::string & regName)
 *  \brief Reads a value from register. Register mask is applied. Will return 0xdeaddead if register is no accessible
 *  \param la Local arguments structure
 *  \param regName Register name
 */
uint32_t readReg(localArgs * la, const std::string & regName);

/*! \fn void writeReg(localArgs * la, const std::string & regName, uint32_t value)
 *  \brief Writes a value to a register. Register mask is applied
 *  \param la Local arguments structure
 *  \param regName Register name
 *  \param value Value to write
 */
void writeReg(localArgs * la, const std::string & regName, uint32_t value);

#endif
