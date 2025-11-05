#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include "Utils.h"
#include "Buffer.h"
#include "system/Filesystem.h"

#define FILE_MAX_FNBUF 64

typedef enum 
{
    FM_RDONLY = LFS_O_RDONLY,
    FM_WRONLY = LFS_O_WRONLY,
    FM_RDWR = LFS_O_WRONLY,
    FM_IO = LFS_O_IO,
    FM_CREATE = LFS_O_CREAT,
    FM_EXCL = LFS_O_EXCL,
    FM_TRUNC = LFS_O_TRUNC,
    FM_APPEND = LFS_O_APPEND
} FileMode;

typedef enum
{
    SEEK_SET = LFS_SEEK_SET, 
    SEEK_CUR = LFS_SEEK_CUR, 
    SEEK_END = LFS_SEEK_END
} FileOrigin;

typedef struct
{
    lfs_file_t f;
    u16 fd;
    Buffer *io_buf;
    char *fname;
} SM_File;

SM_File *F_Open(const char *filename, FileMode openmode);
u8 F_Close(SM_File *file);
u16 F_Read(void *dest, u32 size, u32 count, SM_File *file);
u16 F_Write(void *src, u32 size, u32 count, SM_File *file);
s8 F_Seek(SM_File *file, s32 offset, FileOrigin origin);
u16 F_Tell(SM_File *file);
u16 F_Printf(SM_File *file, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#endif // FILE_H_INCLUDED
