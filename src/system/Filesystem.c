// FS
#include "Filesystem.h"
#include "Utils.h"
#include "Stdout.h"
#include "File.h"
#include "Network.h"

#define MAX_PATH_LENGTH 256

char cwd[MAX_PATH_LENGTH];
static int InitFail = 0;


// Variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

// Read a region in a block. Negative error codes are propagated to the user.
int bd_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    u8 *buf = (u8*)buffer;
    SRAM_enableRO();

    for (lfs_size_t i = 0; i < size; i++)
        buf[i] = SRAM_readByte((block * c->block_size) + off + i);

    SRAM_disable();
    return 0;
}

// Program a region in a block. The block must have previously
// been erased. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int bd_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    u8 *buf = (u8*)buffer;
    SRAM_enable();

    for (lfs_size_t i = 0; i < size; i++)
        SRAM_writeByte((block * c->block_size) + off + i, buf[i]);

    SRAM_disable();
    return 0;
}

// Erase a block. A block must be erased before being programmed.
// The state of an erased block is undefined. Negative error codes are propagated to the user.
// May return LFS_ERR_CORRUPT if the block should be considered bad.
int bd_erase(const struct lfs_config *c, lfs_block_t block)
{
    SRAM_enable();

    for (lfs_size_t i = 0; i < c->block_size; i++)
        SRAM_writeByte((block * c->block_size) + i, 0);

    SRAM_disable();
    return 0;
}

// Sync the state of the underlying block device. Negative error codes are propagated to the user.
int bd_sync(const struct lfs_config *c)
{
    return 0;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = 
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
    .block_count = 128,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = -1,
};

bool FS_EraseSRAM()
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
        printf(" └[92mSRAM detected[0m\n");
        //printf(" └%u KB SRAM detected\n", (SSize+1)/1024);
        //cfg.block_count = (SSize+1) / cfg.block_size; // Set lfs filesystem size to SRAM size

        for (u16 i = 0; i < SSize; i++)
        {
            SRAM_writeByte(i, 0);
        }

        SRAM_disable();
        return TRUE;
    }
    
    printf(" └[91mNo SRAM detected!\n[0m");
    SRAM_disable();
    return FALSE;
}

void FS_Init()
{
    // Mount the filesystem
    InitFail = lfs_mount(&lfs, &cfg);

    // Reformat if we can't mount the filesystem
    // This should only happen on the first boot
    if (InitFail) 
    {
        Stdout_Push("└[91mFilesystem error! Reformatting...[0m\n");
        if (FS_EraseSRAM())
        {
            lfs_format(&lfs, &cfg);
            lfs_mount(&lfs, &cfg);
            FS_MkDir("/system");
            FS_MkDir("/system/tmp");

            /*lfs_file_t f;
            FS_OpenFile("/system/rxbuffer.io", LFS_O_CREAT, &f);
            FS_Close(&f);
            FS_OpenFile("/system/txbuffer.io", LFS_O_CREAT, &f);
            FS_Close(&f);
            FS_OpenFile("/system/stdout.io", LFS_O_CREAT, &f);
            FS_Close(&f);
            FS_OpenFile("/system/stdin.io", LFS_O_CREAT, &f);
            FS_Close(&f);*/

            rxbuf = F_Open("/system/rxbuffer.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDONLY);
            txbuf = F_Open("/system/txbuffer.io", LFS_O_CREAT | LFS_O_TRUNC | LFS_O_WRONLY);
            stdout = F_Open("/system/stdout.io",  LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR);    // LFS_O_WRONLY
            stdin = F_Open("/system/stdin.io",    LFS_O_CREAT | LFS_O_RDONLY);
            stderr = F_Open("/system/stderr.io",  LFS_O_CREAT | LFS_O_TRUNC | LFS_O_RDWR);
        }
    }

    strclr(cwd);
    strcat(cwd, "/");

    return;
}

void FS_PrintBlockDevices()
{
    printf("%5s %6s %12s  %-16s\n", "Name", "Size", "Allocated", "Mount");
    printf("%5s %6ld %12ld  %-16s\n", "SRAM", cfg.block_count*cfg.block_size/1024, lfs_fs_size(&lfs), "/");
}

char *FS_GetCWD()
{
    return cwd;
}

void FS_ListDir(char *dir)
{
    char *path = malloc(64);
    FS_ResolvePath(dir, path);

    lfs_dir_t d;
    u32 err = lfs_dir_open(&lfs, &d, path);

    free(path);
    if (err) return;

    struct lfs_info info;
    char *fbuf = malloc(1024);
    char *dbuf = malloc(1024);
    char *nbuf = malloc(64);

    memset(fbuf, 0, 1024);
    memset(dbuf, 0, 1024);

    while (true) 
    {
        int res = lfs_dir_read(&lfs, &d, &info);
        if (res < 0) goto OnExit;

        if (res == 0) break;

        if (info.name[0] == '.') continue;

        memset(nbuf, 0, 64);

        switch (info.type) 
        {
            case LFS_TYPE_REG:
            {
                u8 nlen = strlen(info.name);
                snprintf(nbuf, 64, "%s%-30s[0m %4lu %s\n", ((info.name[nlen-2] == 'i') && (info.name[nlen-1] == 'o') ? "[95m" : ""), info.name, info.size >= 1024 ? info.size/1024 : info.size, info.size >= 1024 ? "KB" : "B");
                strncat(fbuf, nbuf, 1024);
                break;
            }
            case LFS_TYPE_DIR:
            {
                snprintf(nbuf, 64, "[94m%-30s[0m\n", info.name);
                strncat(dbuf, nbuf, 1024);
                break;
            }

            default:
            {
                snprintf(nbuf, 64, "[91m%-30s[0m %4lu %s\n", info.name, info.size >= 1024 ? info.size/1024 : info.size, info.size >= 1024 ? "KB" : "B");
                strncat(fbuf, nbuf, 1024);
                break;
            }
        }
    }
    
    Stdout_Push(dbuf);
    Stdout_Push(fbuf);

    OnExit:
    free(fbuf);
    free(dbuf);
    free(nbuf);
    lfs_dir_close(&lfs, &d);
    return;
}

int FS_OpenFile(const char *filename, int flags, lfs_file_t *membuf)
{
    return lfs_file_open(&lfs, membuf, filename, flags);
}

lfs_ssize_t FS_ReadFile(void *dest, lfs_ssize_t len, lfs_file_t *membuf)
{
    return lfs_file_read(&lfs, membuf, dest, len);
}

lfs_ssize_t FS_WriteFile(void *src, lfs_ssize_t len, lfs_file_t *membuf)
{
    return lfs_file_write(&lfs, membuf, src, len);
}

int FS_Remove(const char *filename)
{
    return lfs_remove(&lfs, filename);
}

int FS_Rename(const char *old, const char *new)
{
    return lfs_rename(&lfs, old, new);
}

int FS_MkDir(const char *path)
{
    return lfs_mkdir(&lfs, path);
}

lfs_soff_t FS_Seek(lfs_file_t *membuf, lfs_soff_t off, int whence)
{
    return lfs_file_seek(&lfs, membuf, off, whence);
}

lfs_soff_t FS_Tell(lfs_file_t *membuf)
{
    return lfs_file_tell(&lfs, membuf);
}

int FS_Close(lfs_file_t *membuf)
{
    return lfs_file_close(&lfs, membuf);
}

int FS_SetAttr(const char *path, u8 type, const void *buffer, lfs_size_t size)
{
    return lfs_setattr(&lfs, path, type, buffer, size);
}

int FS_GetAttr(const char *path, u8 type, void *buffer, lfs_size_t size)
{
    return lfs_getattr(&lfs, path, type, buffer, size);
}


int path_exists(const char *path) 
{
    lfs_dir_t d;
    u32 err = lfs_dir_open(&lfs, &d, path);
    lfs_dir_close(&lfs, &d);

    return !err;
}

// Function to join two paths, taking care of slashes and buffer limits
void join_paths(char *result, const char *path1, const char *path2) 
{
    if (strlen(path1) + strlen(path2) + 2 > MAX_PATH_LENGTH) 
    {
        printf("Path too long!\n");
        return;
    }
    if (strcmp(path1, "/") == 0) 
    {
        // If path1 is the root, don't add an extra slash
        snprintf(result, MAX_PATH_LENGTH, "/%s", path2);
    } 
    else 
    {
        snprintf(result, MAX_PATH_LENGTH, "%s/%s", path1, path2);
    }
}

// Function to go up one directory level
void FS_GoUpDirectory(char *path)
{
    int len = strlen(path);

    if (len <= 1) 
    {
        strcpy(path, "/"); // Already at root
        return;
    }

    // Traverse backwards to find the last '/'
    for (int i = len - 1; i >= 0; --i) 
    {
        if (path[i] == '/') 
        {
            // End the string here
            path[i] = '\0';
            break;
        }
    }

    // If path becomes empty, set it to root
    if (strlen(path) == 0) 
    {
        strcpy(path, "/");
    }
}

// Function to change the directory
void FS_ChangeDir(const char *path)
{
    char new_path[MAX_PATH_LENGTH];    
    
    // Handle absolute path
    if (path[0] == '/')
    {
        strncpy(new_path, path, MAX_PATH_LENGTH);
    }
    // Handle relative path
    else
    {
        strncpy(new_path, cwd, MAX_PATH_LENGTH);
        char *token = strtok((char *)path, '/');

        while (token != NULL) 
        {
            // Current directory, do nothing
            if (strcmp(token, ".") == 0) 
            {
            }
            // Go up one level
            else if (strcmp(token, "..") == 0) 
            {
                FS_GoUpDirectory(new_path);
            }
            // Go down into a directory
            else
            {
                char temp[MAX_PATH_LENGTH];
                join_paths(temp, new_path, token);
                strncpy(new_path, temp, MAX_PATH_LENGTH);
            }
            token = strtok(NULL, '/');
        }
    }

    // Check if the final path exists
    if (path_exists(new_path)) 
    {
        // Update the current directory
        strncpy(cwd, new_path, MAX_PATH_LENGTH);
    } 
    else 
    {
        printf("Error: Path does not exist.\n");
    }
}

void FS_ResolvePath(const char *path, char *resolved_path) 
{
    // Buffer to build the resolved path
    char temp_path[MAX_PATH_LENGTH];

    // Case 1: Absolute path
    if (path[0] == '/') 
    {
        strncpy(temp_path, path, MAX_PATH_LENGTH);
    }
    // Case 2: Relative path starting with "." or ".."
    else if (path[0] == '.') 
    {
        // Initialize with the current directory
        strncpy(temp_path, cwd, MAX_PATH_LENGTH);

        char *token = strtok((char *)path, '/');
        while (token != NULL) 
        {
            if (strcmp(token, ".") == 0) 
            {
                // Ignore, as it points to the current directory
            } 
            else if (strcmp(token, "..") == 0)
            {
                // Navigate up one level
                FS_GoUpDirectory(temp_path);
            } 
            else
            {
                // Add the directory to the path
                char temp[MAX_PATH_LENGTH];
                join_paths(temp, temp_path, token);
                strncpy(temp_path, temp, MAX_PATH_LENGTH);
            }
            token = strtok(NULL, '/');
        }
    }
    // Case 3: Any other relative path (no . or ..)
    else 
    {
        snprintf(temp_path, MAX_PATH_LENGTH, "%s/%s", cwd, path);
    }

    // Copy the resolved path to the output parameter
    strncpy(resolved_path, temp_path, MAX_PATH_LENGTH);
}
