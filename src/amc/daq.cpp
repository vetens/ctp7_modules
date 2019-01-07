/*!
 * \file amc/daq.cpp
 * \brief AMC DAQ methods for RPC modules
 */

#include "amc/daq.h"

void enableDAQLinkLocal(localArgs* la, uint32_t const& enableMask)
{
  LOGGER->log_message(LogManager::DEBUG, "enableDAQLinkLocal called");
  // writeReg(la, "GEM_AMC.DAQ.CONTROL.INPUT_ENABLE_MASK", enableMask);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAQ_ENABLE", 0x1);
  return;
}

void disableDAQLinkLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.INPUT_ENABLE_MASK", 0x0);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAQ_ENABLE",        0x0);
}

void setZSLocal(localArgs* la, bool en)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.ZERO_SUPPRESSION_EN", uint32_t(en));
}

void disableZSLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.DAQ.CONTROL.ZERO_SUPPRESSION_EN", 0x0);
}

void resetDAQLinkLocal(localArgs* la, uint32_t const& davTO, uint32_t const& ttsOverride)
{
  LOGGER->log_message(LogManager::DEBUG, "resetDAQLinkLocal called");
  writeReg(la, "GEM_AMC.DAQ.CONTROL.RESET", 0x1);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.RESET", 0x0);
  disableDAQLinkLocal(la);
  writeReg(la, "GEM_AMC.DAQ.CONTROL.DAV_TIMEOUT", davTO);
  setDAQLinkInputTimeoutLocal(la);
  // setDAQLinkInputTimeoutLocal(la,davTO);
  // writeReg(la, "GEM_AMC.DAQ.TTS_OVERRIDE", ttsOverride);
  return;
}

uint32_t getDAQLinkControlLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkStatusLocal(localArgs* la)
{
  return 0x0;
}

bool daqLinkReadyLocal(localArgs* la)
{
  return true;
}

bool daqClockLockedLocal(localArgs* la)
{
  return true;
}

bool daqTTCReadyLocal(localArgs* la)
{
  return true;
}

uint8_t daqTTSStateLocal(localArgs* la)
{
  return 0x0;
}

bool daqAlmostFullLocal(localArgs* la)
{
  return true;
}

bool l1aFIFOIsEmptyLocal(localArgs* la)
{
  return true;
}

bool l1aFIFOIsAlmostFullLocal(localArgs* la)
{
  return true;
}

bool l1aFIFOIsFullLocal(localArgs* la)
{
  return true;
}

bool l1aFIFOIsUnderflowLocal(localArgs* la)
{
  return true;
}

uint32_t getDAQLinkEventsSentLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkL1AIDLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkL1ARateLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkDisperErrorsLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkNonidentifiableErrorsLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkInputMaskLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkDAVTimeoutLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkDAVTimerLocal(localArgs* la, bool const& max)
{
  return 0x0;
}

uint32_t getLinkDAQStatusLocal(localArgs* la,    uint8_t const& gtx)
{
  return 0x0;
}

uint32_t getLinkDAQCountersLocal(localArgs* la,  uint8_t const& gtx, uint8_t const& mode)
{
  return 0x0;
}

uint32_t getLinkLastDAQBlockLocal(localArgs* la, uint8_t const& gtx)
{
  return 0x0;
}

uint32_t getDAQLinkInputTimeoutLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkRunTypeLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkRunParametersLocal(localArgs* la)
{
  return 0x0;
}

uint32_t getDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parameter)
{
  return 0x0;
}


void setDAQLinkInputTimeoutLocal(localArgs* la, uint32_t const& inputTO)
{
}

void setDAQLinkRunTypeLocal(localArgs* la, uint32_t const& rtype)
{
}

void setDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parN, uint8_t const& rparam)
{
}

void setDAQLinkRunParametersLocal(localArgs* la, uint32_t const& rparams)
{
}


/** RPC callbacks */
void enableDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t enableMask = request->get_word("enableMask");
  enableDAQLinkLocal(&la, enableMask);
}

void disableDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  disableDAQLinkLocal(&la);
}

void setZS(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool enable = request->get_word("enable");
  setZSLocal(&la,enable);
}

void disableZS(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  disableZSLocal(&la);
}

void resetDAQLink(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  // what if these are not provided, want to use the function defaults, or should these be defined upstream?
  uint32_t davTO       = request->get_word("davTO");
  uint32_t ttsOverride = request->get_word("ttsOverride");
  resetDAQLinkLocal(&la, davTO, ttsOverride);
}

void getDAQLinkControl(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkStatus(const RPCMsg *request, RPCMsg *response)
{
}
void daqLinkReady(const RPCMsg *request, RPCMsg *response)
{
}
void daqClockLocked(const RPCMsg *request, RPCMsg *response)
{
}
void daqTTCReady(const RPCMsg *request, RPCMsg *response)
{
}
void daqTTSState(const RPCMsg *request, RPCMsg *response)
{
}
void daqAlmostFull(const RPCMsg *request, RPCMsg *response)
{
}
void l1aFIFOIsEmpty(const RPCMsg *request, RPCMsg *response)
{
}
void l1aFIFOIsAlmostFull(const RPCMsg *request, RPCMsg *response)
{
}
void l1aFIFOIsFull(const RPCMsg *request, RPCMsg *response)
{
}
void l1aFIFOIsUnderflow(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkEventsSent(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkL1AID(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkL1ARate(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkDisperErrors(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkNonidentifiableErrors(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkInputMask(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkDAVTimeout(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkDAVTimer(const RPCMsg *request, RPCMsg *response)
{
}
void getLinkDAQStatus(const RPCMsg *request, RPCMsg *response)
{
}
void getLinkDAQCounters(const RPCMsg *request, RPCMsg *response)
{
}
void getLinkLastDAQBlock(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response)
{
}
void getDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
}

void setDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t inputTO    = request->get_word("inputTO");
  setDAQLinkInputTimeoutLocal(&la, inputTO);
}

void setDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t runType    = request->get_word("runType");
  setDAQLinkRunTypeLocal(&la, runType);
}

void setDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t parN        = request->get_word("parameterN");
  uint8_t runPar      = request->get_word("runParameter");
  setDAQLinkRunParameterLocal(&la, parN, runPar);
}

void setDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t runPars    = request->get_word("runParameters");
  setDAQLinkRunParametersLocal(&la, runPars);
}

/** Composite RPC methods */
void configureDAQModule(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
}
