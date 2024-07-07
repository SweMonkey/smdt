#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include "Utils.h"

#ifdef KERNEL_BUILD

#include "misc/dosfs.h"

typedef enum e_fmode {FM_READ = 1, FM_WRITE = 2, FM_APPEND = 3} FileMode;
typedef enum e_forigin {SEEK_SET = 0, SEEK_CUR = 1, SEEK_END = 2} FileOrigin;

typedef struct s_file
{
    FILEINFO fi;
    u16 fd;
} SM_File;

SM_File *F_Open(const char *filename, FileMode openmode);
u8 F_Close(SM_File *file);
u16 F_Read(void *dest, u32 size, u32 count, SM_File *file);
u16 F_Write(void *src, u32 size, u32 count, SM_File *file);
s8 F_Seek(SM_File *file, s32 offset, FileOrigin origin);
u16 F_Tell(SM_File *file);

#endif // KERNEL_BUILD

#endif // FILE_H_INCLUDED
