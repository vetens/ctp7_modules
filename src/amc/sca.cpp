/*!
 * \file amc/sca.cpp
 * \brief AMC SCA methods for RPC modules
 */

#include "amc/sca.h"

void scaModuleResetLocal(localArgs* la)
{
  writeReg(la, "GEM_AMC.SLOW_CONTROL.SCA.CTRL.MODULE_RESET", 0x1);
}

void scaHardResetEnableLocal(localArgs* la, bool en)
{
  writeReg(la, "GEM_AMC.SLOW_CONTROL.SCA.CTRL.TTC_HARD_RESET_EN", uint32_t(en));
}

/** RPC callbacks */
void scaModuleReset(const RPCMsg *request, RPCMsg *response)
{
  // struct localArgs la = getLocalArgs(response);
  GETLOCALARGS(response);
}
