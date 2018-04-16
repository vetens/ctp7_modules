/*! \file vfat3.h
 *  \brief RPC module for VFAT3 methods
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef VFAT3_H
#define VFAT3_H
#include "utils.h"
#include <string>

/*! \fn uint32_t vfatSyncCheckLocal(localArgs * la, uint32_t ohN)
 *  \brief Local callable version of vfatSyncCheck
 *  \param la Local arguments structure
 *  \param ohN Optohybrid optical link number
 *  \return Bitmask of sync'ed VFATs
 */
uint32_t vfatSyncCheckLocal(localArgs * la, uint32_t ohN);

/*! \fn void vfatSyncCheck(const RPCMsg *request, RPCMsg *response)
 *  \brief Returns a list of synchronized VFAT chips
 *  \param request RPC request message
 *  \param response RPC responce message
 */
void vfatSyncCheck(const RPCMsg *request, RPCMsg *response);

/*! \fn void configureVFAT3sLocal(localArgs * la, uint32_t ohN, uint32_t vfatMask)
 *  \brief Local callable version of configureVFAT3s
 *  \param la Local arguments structure
 *  \param ohN Optohybrid optical link number
 *  \param vfatMask Bitmask of chip positions determining which chips to use
 */
void configureVFAT3sLocal(localArgs * la, uint32_t ohN, uint32_t vfatMask);

/*! \fn void configureVFAT3s(const RPCMsg *request, RPCMsg *response)
 *  \brief Configures VFAT3 chips
 *
 *  VFAT configurations are sored in files under /mnt/persistent/gemdaq/vfat3/config_OHX_VFATY.txt. Has to be updated later.
 *
 *  \param request RPC request message
 *  \param response RPC responce message
 */
void configureVFAT3s(const RPCMsg *request, RPCMsg *response);

/*! \fn void statusVFAT3sLocal(localArgs * la, uint32_t ohN)
 *  \brief Local callable version of statusVFAT3s
 *  \param la Local arguments structure
 *  \param ohN Optohybrid optical link number
 */
void statusVFAT3sLocal(localArgs * la, uint32_t ohN);

/*! \fn void statusVFAT3s(const RPCMsg *request, RPCMsg *response)
 *  \brief Returns list of values of the most important VFAT3 register 
 *  \param request RPC request message
 *  \param response RPC responce message
 */
void statusVFAT3s(const RPCMsg *request, RPCMsg *response);

#endif
