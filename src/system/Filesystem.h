#ifndef FILESYSTEM_H_INCLUDED
#define FILESYSTEM_H_INCLUDED

#include "Utils.h"
#include "misc/littlefs/lfs.h"

void FS_Init();
void FS_PrintVolInfo();
char *FS_GetCWD();
void FS_PrintBlockDevices();
void FS_ChangeDir(const char *path);
void FS_ListDir(char *dir);

int FS_OpenFile(const char *filename, int flags, lfs_file_t *membuf);
lfs_ssize_t FS_ReadFile(void *dest, lfs_ssize_t len, lfs_file_t *membuf);
lfs_ssize_t FS_WriteFile(void *src, lfs_ssize_t len, lfs_file_t *membuf);
int FS_Remove(const char *filename);
int FS_Rename(const char *old, const char *new);
int FS_MkDir(const char *path);
lfs_soff_t FS_Seek(lfs_file_t *membuf, lfs_soff_t off, int whence);
lfs_soff_t FS_Tell(lfs_file_t *membuf);
int FS_Close(lfs_file_t *membuf);
int FS_SetAttr(const char *path, u8 type, const void *buffer, lfs_size_t size);
int FS_GetAttr(const char *path, u8 type, void *buffer, lfs_size_t size);
void FS_ResolvePath(const char *path, char *resolved_path);

#endif // FILESYSTEM_H_INCLUDED
