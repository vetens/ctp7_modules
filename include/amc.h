/*! \file amc.h
 *  \brief RPC module for amc methods
 *  \author Cameron Bravo <cbravo135@gmail.com>
 *  \author Mykhailo Dalchenko <mykhailo.dalchenko@cern.ch>
 *  \author Brian Dorney <brian.l.dorney@cern.ch>
 */

#ifndef AMC_H
#define AMC_H

#include "utils.h"

namespace amc {

  /*!
   *  \brief Returns AMC FW version
   *         In the case FW version is not 1.X or 3.X, throws std::runtime_error
   *  \param \c caller_name Name of method which called the FW version check FIXME necessary?
   */
  uint32_t fw_version_check(const char* caller_name);

  /*!
   *  \brief returns the vfatMask for the optohybrid ohN
   *  \details Reads the SYNC_ERR_CNT counter for each VFAT on ohN.  If for a given VFAT the counter returns a non-zero value the given VFAT will be masked.
   *  \param ohN Optical link identifier
   */
  struct getOHVFATMask : public xhal::rpc::Method
  {
    uint32_t operator()(const uint32_t &ohN) const;
  };

  /*!
   *  \brief As \c getOHVFATMask but for all optical links specified in ohMask on the AMC
   *
   *  \param ohMask a 12 bit number where a 1 in the n^th bit indicates that the n^th OH should be read back.
   *  \param numOH number of OHs enabled in the mask FIXME redundant
   *  \returns a vector of the VFAT mask for the unmasked OHs
   */
  struct getOHVFATMaskMultiLink : public xhal::rpc::Method
  {
    std::vector<uint32_t> operator()(const uint32_t &ohMask=0xfff, const uint32_t &numOH=0) const;
  };

  /*!
   *  \brief reads out s-bits from OptoHybrid ohN for a number of seconds given by acquireTime and returns them to the user.
   *
   *  \details The s-bit Monitor stores the 8 s-bits that are sent from the OH (they are all sent at the same time
   *           and correspond to the same clock cycle). Each s-bit clusters readout from the s-bit Monitor is a 16-bit word
   *           with bits [0:10] being the sbit address and bits [12:14] being the sbit size, bits 11 and 15 are not used.
   *
   *  \details The possible values of the s-bit Address are [0,1535].  Clusters with address less than 1536 are considered
   *           valid (e.g. there was an sbit); otherwise an invalid (no sbit) cluster is returned.
   *           The s-bit address maps to a given trigger pad following the equation \f$sbit = addr % 64\f$.
   *           There are 64 such trigger pads per VFAT.  Each trigger pad corresponds to two VFAT channels.
   *           The s-bit to channel mapping follows \f$sbit=floor(chan/2)\f$.
   *           You can determine the VFAT position of the sbit via the equation \f$vfatPos=7-int(addr/192)+int((addr%192)/64)*8\f$.
   *  \details The s-bit size represents the number of adjacent trigger pads are part of this cluster.
   *           The s-bit address always reports the lowest trigger pad number in the cluster.
   *           The sbit size takes values [0,7].
   *           So an sbit cluster with address 13 and with size of 2 includes 3 trigger pads for a total of 6
   *           vfat channels and starts at channel \f$13*2=26\f$ and continues to channel \f$(2*15)+1=31\f$.
   *  \details The output vector will always be of size N * 8 where N is the number of readouts of the s-bit Monitor.
   *           For each readout the s-bit Monitor will be reset and then readout after 4095 clock cycles (~102.4 microseconds).
   *           The s-bit clusters will only be added to the output vector if at least one of them was valid.
   *           The s-bit clusters stored in the s-bit Monitor will not be over-written until a module reset is sent.
   *           The readout will stop before acquireTime finishes if the size of the returned vector approaches the max
   *           TCP/IP size (~65000 btyes) and sets maxNetworkSize to true.
   *  \details Each element of the output vector will be a 32 bit word with the following definition
   *           bits [14:26] are the difference between the s-bit and the input L1A (if any) in clock cycles.
   *           bits [11:13] are the cluster size,
   *           bits [0,10] is address of the s-bit Cluster,
   *           While the s-bit monitor stores the difference between the s-bit and input L1A as a 32 bit number (0xFFFFFFFF)
   *           any value higher than 0xFFF (12 bits) will be truncated to 0xFFF.
   *           This matches the time between readouts of 4095 clock cycles.
   *
   *  \param ohN Optical link
   *  \param acquireTime acquisition time on the wall clock in seconds
   *  \param maxNetworkSize pointer to a boolean, set to true if the returned vector reaches a byte count of 65000 FIXME unnecessary?
   */
  struct sbitReadOut : public xhal::rpc::Method
  {
    std::vector<uint32_t> operator()(const uint32_t &ohN, const uint32_t &acquireTime) const;
  };

  /*!
   *  \brief Reads a list of registers for nReads and then counts the number of slow control errors observed.
   *
   *  \param regList is a std::vector<std::string> of registers to be read, note the full node name is required
   *  \param breakOnFailure indicates if a register in \c regList should stop being read after the first failure before \c nReads is reached
   *  \param nReads the number of times each register in \c regList is read
   *
   *  \returns an std::map<std::string,uint32_t> with the following keys, which correspond to the counters under the node "GEM_AMC.SLOW_CONTROL.VFAT3":
   *          "CRC_ERROR_CNT"
   *          "PACKET_ERROR_CNT"
   *          "BITSTUFFING_ERROR_CNT"
   *          "TIMEOUT_ERROR_CNT"
   *          "AXI_STROBE_ERROR_CNT"
   *          "TRANSACTION_CNT"
   *         additionally there will be one final key "SUM" which is the sum of all counters (except TRANSACTION_CNT).
   */
  struct repeatedRegRead : public xhal::rpc::Method
  {
    std::map<std::string,uint32_t> operator()(const std::vector<std::string> &regList, const bool &breakOnFailure=false, const uint32_t &nReads=100) const;
  };
}
#endif
