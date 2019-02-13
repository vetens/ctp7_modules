/*!
 * \file amc/sca.h
 * \brief RPC module for AMC SCA methods, copied in from cmsgemos/gem::hw::HwGenericAMC
 * \author Jared Sturdy <sturdy@cern.ch>
 */

#ifndef AMC_SCA_H
#define AMC_SCA_H

#include "utils.h"

/*!
 * \defgroup sca SCA module functionality
 */

/** Locally executed methods */
/*** CTRL submodule ***/
/*!
 * \brief Reset the SCA module
 */
void scaModuleResetLocal(localArgs* la);

/*!
 * \brief Reset the SCA module
 */
void scaHardResetEnableLocal(localArgs* la, bool en);

/** RPC callbacks */
/*!
 *  \brief RPC method callbacks contain two parameters
 *  \param request RPC request message
 *  \param response RPC response message
 */
void scaModuleReset(const RPCMsg *request, RPCMsg *response);

#endif
