/*!
 * \file amc/ttc.cpp
 * \brief AMC TTC methods for RPC modules
 */

#include "amc/ttc.h"

#include <ios>
#include <iomanip>

void ttcModuleResetLocal(localArgs* la)
{
  // writeReg(la, "GEM_AMC.TTC.CTRL.MODULE_RESET", 0x1);
}

void ttcMMCMResetLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.TTC.CTRL.MMCM_RESET", 0x1);
}

void ttcMMCMPhaseShiftLocal(localArgs* la,
                            bool shiftOutOfLockFirst,
                            bool useBC0Locked,
                            bool doScan)
{
}

int checkPLLLockLocal(localArgs* la, int readAttempts)
{
  LOGGER->log_message(LogManager::WARNING,"checkPLLLock not yet implemented");
  return 0x0;
}

uint32_t getMMCMPhaseMeanLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getMMCMPhaseMean not yet implemented");
  return 0x0;
}

uint32_t getGTHPhaseMeanLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getGTHPhaseMean not yet implemented");
  return 0x0;
}

uint32_t getMMCMPhaseMedianLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getMMCMPhaseMedian not yet implemented");
  return 0x0;
}

uint32_t getGTHPhaseMedianLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getGTHPhaseMedian not yet implemented");
  return 0x0;
}

void ttcCounterResetLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.TTC.CTRL.CNT_RESET", 0x1);
}

bool getL1AEnableLocal(localArgs* la)
{
  return readReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE");
}

void setL1AEnableLocal(localArgs* la,
                       bool enable)
{
  writeReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE", int(enable));
}

/*** CONFIG submodule ***/
uint32_t getTTCConfigLocal(localArgs* la, uint8_t const& cmd)
{
  LOGGER->log_message(LogManager::WARNING,"getTTCConfig not implemented");
  // return readReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE");
  return 0x0;
}

void setTTCConfigLocal(localArgs* la,
                       uint8_t const& cmd,
                       uint8_t const& value)
{
  LOGGER->log_message(LogManager::WARNING,"setTTCConfig not implemented");
  // return writeReg(la, "GEM_AMC.TTC.CTRL.L1A_ENABLE",value);
}

/*** STATUS submodule ***/
uint32_t getTTCStatusLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getTTCStatusLocal not fully implemented");
  // uint32_t retval = readReg(la, "GEM_AMC.TTC.STATUS");
  uint32_t retval = readReg(la, "GEM_AMC.TTC.STATUS.BC0.LOCKED");
  std::stringstream msg;
  msg << "getTTCStatusLocal TTC status reads " << std::hex << std::setw(8) << retval << std::dec;
  LOGGER->log_message(LogManager::DEBUG,msg.str());
  return retval;
}

uint32_t getTTCErrorCountLocal(localArgs* la,
                               bool const& single)
{
  if (single)
    return readReg(la, "GEM_AMC.TTC.STATUS.TTC_SINGLE_ERROR_CNT");
  else
    return readReg(la, "GEM_AMC.TTC.STATUS.TTC_DOUBLE_ERROR_CNT");
}

/*** CMD_COUNTERS submodule ***/
uint32_t getTTCCounterLocal(localArgs* la,
                            uint8_t const& cmd)
{
  switch(cmd) {
  case(0x1) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.L1A");
  case(0x2) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.BC0");
  case(0x3) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.EC0");
  case(0x4) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.RESYNC");
  case(0x5) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.OC0");
  case(0x6) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.HARD_RESET");
  case(0x7) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.CALPULSE");
  case(0x8) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.START");
  case(0x9) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.STOP");
  case(0xa) :
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.TEST_SYNC");
  default :
    std::array<std::string,10> counters = {{"L1A","BC0","EC0","RESYNC","OC0","HARD_RESET","CALPULSE","START","STOP","TEST_SYNC"}};
    for (auto& cnt: counters)
      la->response->set_word(cnt,readReg(la,"GEM_AMC.TTC.CMD_COUNTERS."+cnt));
    return readReg(la,"GEM_AMC.TTC.CMD_COUNTERS.L1A");
  }
}

uint32_t getL1AIDLocal(localArgs* la)
{
  return readReg(la,"GEM_AMC.TTC.L1A_ID");
}

uint32_t getL1ARateLocal(localArgs* la)
{
  return readReg(la,"GEM_AMC.TTC.L1A_RATE");
}

uint32_t getTTCSpyBufferLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getTTCSpyBuffer is obsolete");
  // return readReg(la,"GEM_AMC.TTC.TTC_SPY_BUFFER");
  return 0x0;
}

/** RPC callbacks */
void ttcModuleReset(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  ttcModuleResetLocal(&la);
}

void ttcMMCMReset(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  // LocalArgs la = getLocalArgs(response);
  ttcMMCMResetLocal(&la);
}

void ttcMMCMPhaseShift(const RPCMsg *request, RPCMsg *response)
{
}

void checkPLLLock(const RPCMsg *request, RPCMsg *response)
{
}

void getMMCMPhaseMean(const RPCMsg *request, RPCMsg *response)
{
}

void getMMCMPhaseMedian(const RPCMsg *request, RPCMsg *response)
{
}

void getGTHPhaseMean(const RPCMsg *request, RPCMsg *response)
{
}

void getGTHPhaseMedian(const RPCMsg *request, RPCMsg *response)
{
}

void ttcCounterReset(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  ttcCounterResetLocal(&la);
}
void getL1AEnable(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  getL1AEnableLocal(&la);
}

void setL1AEnable(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool en = request->get_word("enable");
  setL1AEnableLocal(&la, en);
}

void getTTCConfig(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t cmd = request->get_word("cmd");
  getTTCConfigLocal(&la, cmd);
}

void setTTCConfig(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t cmd = request->get_word("cmd");
  uint8_t val = request->get_word("value");
  setTTCConfigLocal(&la, cmd, val);
}

void getTTCStatus(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t status = getTTCStatusLocal(&la);
  // uint32_t status = 0xdeadbeef;
  response->set_word("result",status);
}

void getTTCErrorCount(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool single = request->get_word("single");
  getTTCErrorCountLocal(&la, single);
}

void getTTCCounter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t cmd = request->get_word("cmd");
  getTTCCounterLocal(&la, cmd);
}

void getL1AID(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  getL1AIDLocal(&la);
}

void getL1ARate(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  getL1ARateLocal(&la);
}

void getTTCSpyBuffer(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  getTTCSpyBufferLocal(&la);
}
