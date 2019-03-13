#ifndef BLASTER_RAM_DEFS_H
#define BLASTER_RAM_DEFS_H

static constexpr size_t GBT_RAM_SIZE  = 92;    ///< for GE1/1 3 GBTx per OH, 92 32-bit words of configuration per GBT (366 8-bit GBT configurations -> 32-bit words + padding)
static constexpr size_t VFAT_RAM_SIZE = 74;    ///< for GE1/1 24 VFATs per OH, 74 32-bit words of configuration per VFAT (147 16-bit VFAT configurations -> 32-bit words + padding)
static constexpr size_t OH_RAM_SIZE   = 2*100; ///< for GE1/1 100 32-bit words of configuration per OH plus the corresponding OH local address

///TEMP FIXME!!!!
static constexpr size_t N_GBTX = 3;  ///< 3 GBTx chips per OptoHybrid for GE1/1
static constexpr size_t N_OH   = 12; ///< 12 OptoHybrids per AMC for GE1/1
static constexpr size_t N_VFAT = 24; ///< 24 VFATs per OptoHybrid for GE1/1
///TEMP FIXME!!!!

#endif
