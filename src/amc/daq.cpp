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
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkControl not implemented");
  return 0x0;
}

uint32_t getDAQLinkStatusLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkStatus not implemented");
  return 0x0;
}

bool daqLinkReadyLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"daqLinkReady not implemented");
  return true;
}

bool daqClockLockedLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"daqClockLocked not implemented");
  return true;
}

bool daqTTCReadyLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"daqTTCReady not implemented");
  return true;
}

uint8_t daqTTSStateLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"daqTTSState not implemented");
  return 0x0;
}

bool daqAlmostFullLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"daqAlmostFull not implemented");
  return true;
}

bool l1aFIFOIsEmptyLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"l1aFIFOIsEmpty not implemented");
  return true;
}

bool l1aFIFOIsAlmostFullLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"l1aFIFOIsAlmostFull not implemented");
  return true;
}

bool l1aFIFOIsFullLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"l1aFIFOIsFull not implemented");
  return true;
}

bool l1aFIFOIsUnderflowLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"l1aFIFOIsUnderflow not implemented");
  return true;
}

uint32_t getDAQLinkEventsSentLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkEventsSent not implemented");
  return 0x0;
}

uint32_t getDAQLinkL1AIDLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkL1AID not implemented");
  return 0x0;
}

uint32_t getDAQLinkL1ARateLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkL1ARate not implemented");
  return 0x0;
}

uint32_t getDAQLinkDisperErrorsLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkDisperErrors not implemented");
  return 0x0;
}

uint32_t getDAQLinkNonidentifiableErrorsLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkNonidentifiableErrors not implemented");
  return 0x0;
}

uint32_t getDAQLinkInputMaskLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkInputMask not implemented");
  return 0x0;
}

uint32_t getDAQLinkDAVTimeoutLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkDAVTimeout not implemented");
  return 0x0;
}

uint32_t getDAQLinkDAVTimerLocal(localArgs* la, bool const& max)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkDAVTimer not implemented");
  return 0x0;
}

uint32_t getLinkDAQStatusLocal(localArgs* la,    uint8_t const& gtx)
{
  LOGGER->log_message(LogManager::WARNING,"getLinkDAQStatus not implemented");
  return 0x0;
}

uint32_t getLinkDAQCountersLocal(localArgs* la,  uint8_t const& gtx, uint8_t const& mode)
{
  LOGGER->log_message(LogManager::WARNING,"getLinkDAQCounters not implemented");
  return 0x0;
}

uint32_t getLinkLastDAQBlockLocal(localArgs* la, uint8_t const& gtx)
{
  LOGGER->log_message(LogManager::WARNING,"getLinkLastDAQBlock not implemented");
  return 0x0;
}

uint32_t getDAQLinkInputTimeoutLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkInputTimeout not implemented");
  return 0x0;
}

uint32_t getDAQLinkRunTypeLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkRunType not implemented");
  return 0x0;
}

uint32_t getDAQLinkRunParametersLocal(localArgs* la)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkRunParameters not implemented");
  return 0x0;
}

uint32_t getDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parameter)
{
  LOGGER->log_message(LogManager::WARNING,"getDAQLinkRunParameter not implemented");
  return 0x0;
}


void setDAQLinkInputTimeoutLocal(localArgs* la, uint32_t const& inputTO)
{
  LOGGER->log_message(LogManager::WARNING,"setDAQLinkInputTimeout not implemented");
}

void setDAQLinkRunTypeLocal(localArgs* la, uint32_t const& rtype)
{
  writeReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_TYPE", rtype);
}

void setDAQLinkRunParameterLocal(localArgs* la, uint8_t const& parN, uint8_t const& rparam)
{
  if (parN < 1 || parN > 3) {
    std::stringstream errmsg;
    errmsg << "Attempting to set DAQ link run parameter " << parN << ": outside expectation (1-3)";
    la->response->set_string("error", errmsg.str());
    return;
  }
  std::stringstream regBase;
  regBase << "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAM" << (int)parN;
  writeReg(la, regBase.str(), rparam);
}

void setDAQLinkRunParametersLocal(localArgs* la, uint32_t const& rparams)
{
  writeReg(la, "GEM_AMC.DAQ.EXT_CONTROL.RUN_PARAMS", rparams);
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
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkControlLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkStatus(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkStatusLocal(&la);
  response->set_word("result", res);
}

void daqLinkReady(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqLinkReadyLocal(&la);
  response->set_word("result", res);
}

void daqClockLocked(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqClockLockedLocal(&la);
  response->set_word("result", res);
}

void daqTTCReady(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = daqTTCReadyLocal(&la);
  response->set_word("result", res);
}

void daqTTSState(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = daqTTSStateLocal(&la);
  response->set_word("result", res);
}

void daqAlmostFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = daqAlmostFullLocal(&la);
  response->set_word("result", res);
}

void l1aFIFOIsEmpty(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsEmptyLocal(&la);
  response->set_word("result", res);
}

void l1aFIFOIsAlmostFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsAlmostFullLocal(&la);
  response->set_word("result", res);
}

void l1aFIFOIsFull(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsFullLocal(&la);
  response->set_word("result", res);
}

void l1aFIFOIsUnderflow(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool res = l1aFIFOIsUnderflowLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkEventsSent(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkEventsSentLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkL1AID(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkL1AIDLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkL1ARate(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkL1ARateLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkDisperErrors(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkDisperErrorsLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkNonidentifiableErrors(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkNonidentifiableErrorsLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkInputMask(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkInputMaskLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkDAVTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkDAVTimeoutLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkDAVTimer(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  bool max = request->get_word("max");
  uint32_t res = getDAQLinkDAVTimerLocal(&la, max);
  response->set_word("result", res);
}

void getLinkDAQStatus(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx = request->get_word("gtx");
  uint32_t res = getLinkDAQStatusLocal(&la, gtx);
  response->set_word("result", res);
}

void getLinkDAQCounters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx  = request->get_word("gtx");
  uint8_t mode = request->get_word("mode");
  uint32_t res = getLinkDAQCountersLocal(&la, gtx, mode);
  response->set_word("result", res);
}

void getLinkLastDAQBlock(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t gtx  = request->get_word("gtx");
  uint32_t res = getLinkLastDAQBlockLocal(&la, gtx);
  response->set_word("result", res);
}

void getDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkInputTimeoutLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkRunTypeLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkRunParameters(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t res = getDAQLinkRunParametersLocal(&la);
  response->set_word("result", res);
}

void getDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t parameter  = request->get_word("parameter");
  uint32_t res = getDAQLinkRunParameterLocal(&la, parameter);
  response->set_word("result", res);
}

void setDAQLinkInputTimeout(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t inputTO = request->get_word("inputTO");
  setDAQLinkInputTimeoutLocal(&la, inputTO);
}

void setDAQLinkRunType(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint32_t runType = request->get_word("runType");
  setDAQLinkRunTypeLocal(&la, runType);
}

void setDAQLinkRunParameter(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
  uint8_t parN   = request->get_word("parameterN");
  uint8_t runPar = request->get_word("runParameter");
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
