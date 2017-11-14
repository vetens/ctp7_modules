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

void writeRawAddress(uint32_t address, uint32_t value, RPCMsg *response){
  uint32_t data[1]; 
  data[0] = value;
  if (memsvc_write(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::INFO, stdsprintf("write memsvc error: %s", memsvc_get_last_error(memsvc)));
  }
}

uint32_t readRawAddress(uint32_t address, RPCMsg* response){
  uint32_t data[1];
  if (memsvc_read(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
    return 0xdeaddead;
  }
  return data[0];
}

uint32_t getAddress(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, RPCMsg *response){
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = dbi.get(rtxn,key,db_res);
  uint32_t address;
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    address = stoi(tmp[0]);
  } else {
    LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
  return address;
}

void writeAddress(lmdb::val & db_res, uint32_t value, RPCMsg *response) {
  std::vector<std::string> tmp;
  std::string t_db_res = std::string(db_res.data());
  t_db_res = t_db_res.substr(0,db_res.size());
  tmp = split(t_db_res,'|');
  uint32_t data[1];
  uint32_t address = stoi(tmp[0]);
  data[0] = value;
  if (memsvc_write(memsvc, address, 1, data) != 0) {
  	response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
  	LOGGER->log_message(LogManager::INFO, stdsprintf("write memsvc error: %s", memsvc_get_last_error(memsvc)));
  }
}

uint32_t readAddress(lmdb::val & db_res, RPCMsg *response) {
  std::vector<std::string> tmp;
  std::string t_db_res = std::string(db_res.data());
  t_db_res = t_db_res.substr(0,db_res.size());
  tmp = split(t_db_res,'|');
  uint32_t data[1];
  uint32_t address = stoi(tmp[0]);
  int n_current_tries = 0;
  while (true) 
  {
      if (memsvc_read(memsvc, address, 1, data) != 0) 
      {
          if (n_current_tries < 9) 
          {
              n_current_tries++;
              LOGGER->log_message(LogManager::ERROR, stdsprintf("Reading reg %08X failed %i times.", address, n_current_tries));
          }
          else
          {
               response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
               LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s failed 10 times", memsvc_get_last_error(memsvc)));
               return 0xdeaddead;
          }
      }
      else break;
  }
  return data[0];
}

void writeRawReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, uint32_t value, RPCMsg *response) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = dbi.get(rtxn,key,db_res);
  if (found){
    writeAddress(db_res, value, response);
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    response->set_string("error", "Register not found");
  }
}

uint32_t readRawReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, RPCMsg *response) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = dbi.get(rtxn,key,db_res);
  if (found){
    return readAddress(db_res, response);
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
}

uint32_t applyMask(uint32_t data, uint32_t mask) {
  uint32_t result = data & mask;
  for (int i = 0; i < 32; i++)
  {
    if (mask & 1) 
    {
      break;
    }else {
      mask = mask >> 1;
      result = result >> 1;
    }
  }
  return result;
}

uint32_t readReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = dbi.get(rtxn,key,db_res);
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    std::size_t found = tmp[1].find_first_of("r");
    if (found==std::string::npos) {
    	//response->set_string("error", std::string("No read permissions"));
    	LOGGER->log_message(LogManager::ERROR, stdsprintf("No read permissions for %s", regName.c_str()));
      return 0xdeaddead;
    }
    uint32_t data[1];
    uint32_t address,mask;
    address = stoll(tmp[0]);
    mask = stoll(tmp[2]);
    if (memsvc_read(memsvc, address, 1, data) != 0) {
    	//response->set_string("error", std::string("memsvc error: ")+memsvc_get_last_error(memsvc));
    	LOGGER->log_message(LogManager::ERROR, stdsprintf("read memsvc error: %s", memsvc_get_last_error(memsvc)));
      return 0xdeaddead;
    }
    if (mask!=0xFFFFFFFF) {
      return applyMask(data[0],mask);
    } else {
      return data[0];
    }
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    //response->set_string("error", "Register not found");
    return 0xdeaddead;
  }
}

void writeReg(lmdb::txn & rtxn, lmdb::dbi & dbi, const std::string & regName, uint32_t value, RPCMsg *response) {
  lmdb::val key, db_res;
  bool found;
  key.assign(regName.c_str());
  found = dbi.get(rtxn,key,db_res);
  if (found){
    std::vector<std::string> tmp;
    std::string t_db_res = std::string(db_res.data());
    t_db_res = t_db_res.substr(0,db_res.size());
    tmp = split(t_db_res,'|');
    uint32_t mask = stoll(tmp[2]);
    if (mask==0xFFFFFFFF) {
      writeAddress(db_res, value, response);
    } else {
      uint32_t current_value = readAddress(db_res, response);
      if (current_value == 0xdeaddead) {
  	    response->set_string("error", std::string("Writing masked reg failed due to reading problem"));
  	    LOGGER->log_message(LogManager::ERROR, stdsprintf("Writing masked reg failed due to reading problem: %s", regName.c_str()));
        return;
      }
      int shift_amount = 0; 
      uint32_t mask_copy = mask;
      for (int i = 0; i < 32; i++)
      {
        if (mask & 1) 
        {
          break;
        } else {
          shift_amount +=1;
          mask = mask >> 1;
        }
      }
      uint32_t val_to_write = value << shift_amount;
      val_to_write = (val_to_write & mask_copy) | (current_value & ~mask_copy);
      writeAddress(db_res, val_to_write, response);
    }
  } else {
  	LOGGER->log_message(LogManager::ERROR, stdsprintf("Key: %s is NOT found", regName.c_str()));
    response->set_string("error", "Register not found");
  }
}

#endif
