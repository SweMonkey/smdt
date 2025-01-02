#include "File.h"
#include "Filesystem.h"
#include "Utils.h"

#define MAX_FD 64
#define MAX_PRINTF_BUF 256

static u16 FD_Count = 0;    // Open file descriptors


SM_File *F_GetFreeFD()
{
    if (FD_Count >= MAX_FD) return NULL;

    SM_File *file = (SM_File*)malloc(sizeof(SM_File));

    if (file != NULL) FD_Count++;

    return file;
}

SM_File *F_Open(const char *filename, FileMode openmode)
{
    SM_File *file = F_GetFreeFD();

    if (file == NULL)
    {
        printf("No free filedescriptors available!\n");
        return NULL;
    }

    s32 r = FS_OpenFile(filename, openmode, &file->f);

    if (r)
    {
        //printf("Error opening file \"%s\"\nError code $%lX (%ld)\n", filename, r, r);
        return NULL;
    }

    //printf("Opened file \"%s\"\n", filename);

    return file;
}

u8 F_Close(SM_File *file)
{
    if (file == NULL) 
    {
        return -1; // EOF
    }

    FS_Close(&file->f);

    free(file);
    file = NULL;

    FD_Count--;

    return 0;    
}

u16 F_Read(void *dest, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_RDONLY))
    {
        return FS_ReadFile(dest, size*count, &file->f);
    }

    return 0;    
}

u16 F_Write(void *src, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_WRONLY))
    {
        return FS_WriteFile(src, size*count, &file->f);
    }

    return 0;    
}

s8 F_Seek(SM_File *file, s32 offset, FileOrigin origin)
{
    return FS_Seek(&file->f, offset, origin);
}

u16 F_Tell(SM_File *file)
{
    return FS_Tell(&file->f);
}

u16 vsnprintf(char *buf, u16 size, const char *fmt, va_list args);
u16 F_Printf(SM_File *file, const char *fmt, ...)
{
    va_list args;
    u16 i;
    char *buffer = malloc(MAX_PRINTF_BUF);

    va_start(args, fmt);
    i = vsnprintf(buffer, MAX_PRINTF_BUF, fmt, args);
    va_end(args);

    F_Write(buffer, i, 1, file);

    free(buffer);

    return i;
}
