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

/*! \fn void setChannelRegistersVFAT3Local(localArgs * la, uint32_t ohN, uint32_t vfatMask, uint32_t *calEnable, uint32_t *masks, uint32_t *trimARM, uint32_t *trimARMPol, uint32_t *trimZCC, uint32_t *trimZCCPol);
  + *  \brief writes all vfat3 channel registers from AMC
  + *  \param ohN Optohybrid optical link number
  + *  \param vfatMask Bitmask of chip positions determining which chips to use
  + *  \param calEnable array pointer for calEnable with 3072 entries, the (vfat,chan) pairing determines the array index via: idx = vfat*128 + chan
  + *  \param masks as calEnable but for channel masks
  + *  \param trimARM as calEnable but for arming comparator trim value
  + *  \param trimARMPol as calEnable but for arming comparator trim polarity
  + *  \param trimZCC as calEnable but for zero crossing comparator trim value
  + *  \param trimZCCPol as calEnable but for zero crossing comparator trim polarity
  + */
void setChannelRegistersVFAT3Local(localArgs * la, uint32_t ohN, uint32_t vfatMask, uint32_t *calEnable, uint32_t *masks, uint32_t *trimARM, uint32_t *trimARMPol, uint32_t *trimZCC, uint32_t *trimZCCPol);

/*! \fn void setChannelRegistersVFAT3(const RPCMsg *request, RPCMsg *response);
  + *  \brief writes all vfat3 channel registers from host machine
  + *  \param request RPC request message
  + *  \param response RPC responce message
  + */
void setChannelRegistersVFAT3(const RPCMsg *request, RPCMsg *response);

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
