#ifndef FS_SRAM_H_INCLUDED
#define FS_SRAM_H_INCLUDED

#include "misc/littlefs/lfs.h"

extern lfs_t lfs_SRAM;
extern const struct lfs_config cfg_sram;

bool FS_Mount_SRAM();
bool FS_Erase_SRAM();

#endif // FS_SRAM_H_INCLUDED
