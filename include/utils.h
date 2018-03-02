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

memsvc_handle_t memsvc;

struct localArgs {
    lmdb::txn & rtxn;
    lmdb::dbi & dbi;
    RPCMsg *response;
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

void writeRawAddress(uint32_t address, uint32_t value, RPCMsg *response);

uint32_t readRawAddress(uint32_t address, RPCMsg* response);

uint32_t getAddress(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, RPCMsg *response);

void writeAddress(lmdb::val & db_res, uint32_t value, RPCMsg *response);

uint32_t readAddress(lmdb::val & db_res, RPCMsg *response);

void writeRawReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, uint32_t value, RPCMsg *response);

uint32_t readRawReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, RPCMsg *response);

uint32_t applyMask(uint32_t data, uint32_t mask);

uint32_t readReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName);

void writeReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, uint32_t value, RPCMsg *response);

#endif
