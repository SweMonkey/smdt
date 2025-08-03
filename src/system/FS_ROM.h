#ifndef FS_ROM_H_INCLUDED
#define FS_ROM_H_INCLUDED

#include "misc/littlefs/lfs.h"

extern lfs_t lfs_ROM;
extern const struct lfs_config cfg_rom;

bool FS_Mount_ROM();

#endif // FS_ROM_H_INCLUDED
