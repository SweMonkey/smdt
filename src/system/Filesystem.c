#include "Filesystem.h"
#include "Utils.h"
#include "Stdout.h"
#include "File.h"
#include "FS_ROM.h"
#include "FS_SRAM.h"

#define NUM_PARTITIONS 2
#define MAX_PATH_LENGTH 256
char cwd[MAX_PATH_LENGTH];
char full_cwd[MAX_PATH_LENGTH];
u16 ps = 1;                 // Selected partition

typedef struct
{
    lfs_t        *lfs_type;
    const char   *RootDir;
    const char   *PartName;
    BoolCallback *MountFunc;
    BoolCallback *FormatFunc;
    const struct lfs_config *cfg;
} FS_PartList;

static const FS_PartList PList[] =
{
    {&lfs_ROM , "/rom" , "ROM" , FS_Mount_ROM , NULL         , &cfg_rom },
    {&lfs_SRAM, "/sram", "SRAM", FS_Mount_SRAM, FS_Erase_SRAM, &cfg_sram}
};

void FS_SetActivePartition(u16 num);


void FS_Init()
{
    bool r = 1;
    for (u8 i = 0; i < NUM_PARTITIONS; i++)
    {
        r = PList[i].MountFunc();
    }

    memset(cwd, 0, MAX_PATH_LENGTH);
    cwd[0] = '/';

    u16 p = (r == 1) ? 0 : 1;   // If last mounted partition is the SRAM one and mount failed then set active partition to the ROM fs
    FS_SetActivePartition(p);

    return;
}

void FS_SetActivePartition(u16 num)
{
    if (num >= NUM_PARTITIONS) return;

    ps = num;
}

u16 FS_GetActivePartition()
{
    return ps;
}

const char *FS_GetPartitionFromDir(const char *dir, u16 *old_part)
{
    static char buffer[MAX_PATH_LENGTH];

    if (old_part != NULL) *old_part = FS_GetActivePartition();

    if (dir == NULL) return NULL;

    for (u16 i = 0; i < sizeof(PList) / sizeof(PList[0]); ++i)
    {
        const char *root = PList[i].RootDir;
        size_t len = strlen(root);

        if (strncmp(dir, root, len) == 0 && (dir[len] == '/' || dir[len] == '\0'))
        {
            FS_SetActivePartition(i);

            const char *suffix = dir + len;
            const char *new_dir = (*suffix == '\0') ? "/" : suffix;

            strncpy(buffer, new_dir, MAX_PATH_LENGTH - 1);
            buffer[MAX_PATH_LENGTH - 1] = '\0';
            return buffer;
        }
    }

    // In case no partiton just return dir as is
    return dir;
}


const char *FS_GetRootDir()
{
    return PList[ps].RootDir;
}

char *FS_GetCWD()
{
    memset(full_cwd, 0, MAX_PATH_LENGTH);
    strcat(full_cwd, PList[ps].RootDir);
    strcat(full_cwd, cwd);
    
    return full_cwd;
}

void FS_PrintBlockDevices()
{
    printf("%3s %5s %6s %10s  %-10s\n", "Num", "Name", "Size", "Allocated", "Mount");

    for (u8 i = 0; i < NUM_PARTITIONS; i++)
    {
        printf("%3d %5s %5ldK %9ldK  %-10s\n", i, PList[i].PartName, (PList[i].lfs_type->block_count * PList[i].cfg->block_size)/1024, (lfs_fs_size(PList[i].lfs_type) * PList[i].cfg->block_size)/1024, PList[i].RootDir);
    }
}

void FS_ListDir(char *dir)
{
    char *path = malloc(64);
    u16 old_part;
    const char *spath = FS_GetPartitionFromDir(dir, &old_part);
    FS_ResolvePath(spath, path);

    lfs_dir_t d;
    u32 err = lfs_dir_open(PList[ps].lfs_type, &d, path);

    free(path);
    if (err) goto OnError;

    struct lfs_info info;
    char *fbuf = malloc(1024);
    char *dbuf = malloc(1024);
    char *nbuf = malloc(64);

    memset(fbuf, 0, 1024);
    memset(dbuf, 0, 1024);

    while (true) 
    {
        int res = lfs_dir_read(PList[ps].lfs_type, &d, &info);
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
        lfs_dir_close(PList[ps].lfs_type, &d);
    OnError:
        FS_SetActivePartition(old_part);
    return;
}

int FS_OpenFile(const char *filename, int flags, lfs_file_t *membuf)
{
    u16 old_part;
    const char *sfilename = FS_GetPartitionFromDir(filename, &old_part);

    int r = lfs_file_open(PList[ps].lfs_type, membuf, sfilename, flags);

    FS_SetActivePartition(old_part);
    return r;
}

lfs_ssize_t FS_ReadFile(void *dest, lfs_ssize_t len, lfs_file_t *membuf, const char *fn)
{
    u16 old_part;
    FS_GetPartitionFromDir(fn, &old_part);

    lfs_ssize_t r = lfs_file_read(PList[ps].lfs_type, membuf, dest, len);

    FS_SetActivePartition(old_part);
    return r;
}

lfs_ssize_t FS_WriteFile(void *src, lfs_ssize_t len, lfs_file_t *membuf, const char *fn)
{
    u16 old_part;
    FS_GetPartitionFromDir(fn, &old_part);

    lfs_ssize_t r = lfs_file_write(PList[ps].lfs_type, membuf, src, len);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_Remove(const char *filename)
{
    u16 old_part;
    const char *sfilename = FS_GetPartitionFromDir(filename, &old_part);

    int r = lfs_remove(PList[ps].lfs_type, sfilename);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_Rename(const char *old, const char *new)
{
    u16 old_part;
    const char *sold = FS_GetPartitionFromDir(old, &old_part);

    int r = lfs_rename(PList[ps].lfs_type, sold, new);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_MkDir(const char *path)
{
    u16 old_part;
    const char *spath = FS_GetPartitionFromDir(path, &old_part);

    int r = lfs_mkdir(PList[ps].lfs_type, spath);

    FS_SetActivePartition(old_part);
    return r;
}

lfs_soff_t FS_Seek(lfs_file_t *membuf, lfs_soff_t off, int whence, const char *fn)
{
    u16 old_part;
    FS_GetPartitionFromDir(fn, &old_part);

    lfs_soff_t r = lfs_file_seek(PList[ps].lfs_type, membuf, off, whence);

    FS_SetActivePartition(old_part);
    return r;
}

lfs_soff_t FS_Tell(lfs_file_t *membuf, const char *fn)
{
    u16 old_part;
    FS_GetPartitionFromDir(fn, &old_part);

    lfs_soff_t r = lfs_file_tell(PList[ps].lfs_type, membuf);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_Close(lfs_file_t *membuf, const char *fn)
{
    u16 old_part;
    FS_GetPartitionFromDir(fn, &old_part);

    int r = lfs_file_close(PList[ps].lfs_type, membuf);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_SetAttr(const char *path, u8 type, const void *buffer, lfs_size_t size)
{
    u16 old_part;
    const char *spath = FS_GetPartitionFromDir(path, &old_part);

    int r = lfs_setattr(PList[ps].lfs_type, spath, type, buffer, size);

    FS_SetActivePartition(old_part);
    return r;
}

int FS_GetAttr(const char *path, u8 type, void *buffer, lfs_size_t size)
{
    u16 old_part;
    const char *spath = FS_GetPartitionFromDir(path, &old_part);

    int r = lfs_getattr(PList[ps].lfs_type, spath, type, buffer, size);

    FS_SetActivePartition(old_part);
    return r;
}


int path_exists(const char *path) 
{
    lfs_dir_t d;
    u32 err = lfs_dir_open(PList[ps].lfs_type, &d, path);
    lfs_dir_close(PList[ps].lfs_type, &d);

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
    
    const char *old_path = FS_GetPartitionFromDir(path, NULL);
    
    // Handle absolute path
    if (old_path[0] == '/')
    {
        strncpy(new_path, old_path, MAX_PATH_LENGTH);
    }
    // Handle relative path
    else
    {
        strncpy(new_path, cwd, MAX_PATH_LENGTH);
        char *token = strtok((char *)old_path, '/');

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
