#include "Filesystem.h"
#include "FS_ROM.h"
#include "Stdout.h"
#include "../res/system.h"

lfs_t lfs_ROM;


static int bd_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    u8 *buf = (u8*)buffer;

    for (lfs_size_t i = 0; i < size; i++)
        buf[i] = ROM_DISK[(block * c->block_size) + off + i];

    return LFS_ERR_OK;
}

static int bd_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    return LFS_ERR_NOSPC;
}

static int bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    return LFS_ERR_OK;
}

static int bd_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

const struct lfs_config cfg_rom = 
{
    // block device operations
    .read  = bd_read,
    .prog  = bd_prog,
    .erase = bd_erase,
    .sync  = bd_sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = 256,
    .block_count = 0,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = -1,
};

bool FS_Mount_ROM()
{
    // Mount the ROM filesystem
    int InitFail = lfs_mount(&lfs_ROM, &cfg_rom);

    if (InitFail) 
    {
        Stdout_Push(" [91mFilesystem error! Can't mount ROM_DSK[0m\n");
        return 1;
    }

    Stdout_Push(" [92mROM_DISK successfully mounted[0m\n");
    return 0;
}
