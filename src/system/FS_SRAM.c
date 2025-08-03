#include "Filesystem.h"
#include "FS_SRAM.h"
#include "Stdout.h"
#include "Network.h"

lfs_t lfs_SRAM;


// Read a region in a block. Negative error codes are propagated to the user.
static int bd_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    u8 *buf = (u8*)buffer;
    SRAM_enableRO();

    for (lfs_size_t i = 0; i < size; i++)
        buf[i] = SRAM_readByte((block * c->block_size) + off + i);

    SRAM_disable();
    return LFS_ERR_OK;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int bd_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    u8 *buf = (u8*)buffer;
    SRAM_enable();

    for (lfs_size_t i = 0; i < size; i++)
        SRAM_writeByte((block * c->block_size) + off + i, buf[i]);

    SRAM_disable();
    return LFS_ERR_OK;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
static int bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    SRAM_enable();

    for (lfs_size_t i = 0; i < c->block_size; i++)
        SRAM_writeByte((block * c->block_size) + i, 0);

    SRAM_disable();
    return LFS_ERR_OK;
}

// Sync the state of the underlying block device. Negative error codes are propagated to the user.
static int bd_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg_sram = 
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
    .block_count = 128, // Do not set to 0, reformatting will fail!
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = -1,
};

bool FS_Mount_SRAM()
{
    // Mount the SRAM filesystem
    int InitFail = lfs_mount(&lfs_SRAM, &cfg_sram);

    // Reformat if we can't mount the filesystem
    // This should only happen on the first boot
    if (InitFail) 
    {
        printf(" [91mSRAM filesystem error: %d\n Reformatting partition...[0m\n", InitFail);
        if (FS_Erase_SRAM())
        {
            lfs_format(&lfs_SRAM, &cfg_sram);
            InitFail = lfs_mount(&lfs_SRAM, &cfg_sram);

            // Error, bail
            if (InitFail) 
            {
                printf(" [91mSRAM reformatting error: %d[0m\n", InitFail);
                goto Error;
            }

            FS_MkDir("/sram/system");
            FS_MkDir("/sram/system/tmp");

            rxbuf = F_Open("/sram/system/rxbuffer.io", LFS_O_CREAT | LFS_O_TRUNC  | LFS_O_RDONLY | LFS_O_IO);
            txbuf = F_Open("/sram/system/txbuffer.io", LFS_O_CREAT | LFS_O_TRUNC  | LFS_O_WRONLY | LFS_O_IO);
            stdout = F_Open("/sram/system/stdout.io",  LFS_O_CREAT | LFS_O_TRUNC  | LFS_O_RDWR   | LFS_O_IO);    // LFS_O_WRONLY
            stdin = F_Open("/sram/system/stdin.io",    LFS_O_CREAT | LFS_O_RDONLY | LFS_O_IO);
            stderr = F_Open("/sram/system/stderr.io",  LFS_O_CREAT | LFS_O_TRUNC  | LFS_O_RDWR   | LFS_O_IO);

            goto Success;
        }

        goto Error;
    }

    Success:
    Stdout_Push(" [92mSRAM_DISK successfully mounted[0m\n");
    return 0;

    Error:
    Stdout_Push(" [91mFailed to mount SRAM_DISK[0m\n");
    return 1;
}

bool FS_Erase_SRAM()
{
    vu32 *SStart = (u32*)0x1B4;
    vu32 *SEnd = (u32*)0x1B8;
    u16 SSize = (*SEnd-*SStart) >> 1;

    SRAM_enable();

    // Check if SRAM is present
    SRAM_writeByte(0, 0xDE);
    SRAM_writeByte(1, 0xAD);
    SRAM_writeByte(SSize-2, 0xBE);
    SRAM_writeByte(SSize-1, 0xEF);

    u8 r1 = SRAM_readByte(0);
    u8 r2 = SRAM_readByte(1);
    u8 r3 = SRAM_readByte(SSize-2);
    u8 r4 = SRAM_readByte(SSize-1);

    // Erase SRAM if present
    if ((u32)((r1 << 24) | (r2 << 16) | (r3 << 8) | (r4)) == 0xDEADBEEF)
    {
        printf(" [92mSRAM detected[0m\n");

        for (u16 i = 0; i < SSize; i++)
        {
            SRAM_writeByte(i, 0);
        }

        SRAM_disable();
        return TRUE;
    }
    
    printf(" [91mNo SRAM detected!\n[0m");
    SRAM_disable();
    return FALSE;
}
