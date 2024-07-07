#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED

#include "Utils.h"

#ifdef KERNEL_BUILD

#include "misc/dosfs.h"

void FS_Init();
void FS_PrintVolInfo();
void FS_ListDir(char *dir);

u32 FS_OpenFile(const char *filename, u8 openmode, PFILEINFO fi);
u32 FS_ReadFile(PFILEINFO fi, void *dest, u32 len);
u32 FS_WriteFile(PFILEINFO fi, void *src, u32 len);
u8 FS_Unlink(const char *filename);

#endif // KERNEL_BUILD

#endif // FILESYSTEM_H_INCLUDED
