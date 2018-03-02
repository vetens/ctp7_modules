#ifndef VFAT3_H
#define VFAT3_H
#include "utils.h"
#include <string>

uint32_t vfatSyncCheckLocal(localArgs * la, uint32_t ohN);

void vfatSyncCheck(const RPCMsg *request, RPCMsg *response);

void configureVFAT3sLocal(localArgs * la, uint32_t ohN, uint32_t vfatMask);

void configureVFAT3s(const RPCMsg *request, RPCMsg *response);

void statusVFAT3sLocal(localArgs * la, uint32_t ohN);

void statusVFAT3s(const RPCMsg *request, RPCMsg *response);

#endif
