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

    file->fname = NULL;

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
        F_Close(file);  // Close file to avoid leaking memory...
        //printf("Error opening file \"%s\"\nError code $%lX (%ld)\n", filename, r, r);
        return NULL;
    }

    size_t len = strlen(filename);
    file->fname = malloc(len + 1);
    if (file->fname) 
    {
        strncpy(file->fname, filename, len);
        file->fname[len] = '\0';
    }

    //kprintf("Opened file \"%s\"", filename);

    return file;
}

u8 F_Close(SM_File *file)
{
    if (file == NULL) 
    {
        return -1; // EOF
    }

    int r = FS_Close(&file->f, file->fname);

    if (r)
    {
        //printf("Error closing file \"%s\"\nError code $%lX (%ld)\n", file->fname, r, r);
    }

    free(file->fname);
    file->fname = NULL;

    free(file);
    file = NULL;

    FD_Count--;

    //kprintf("Closed file");
    return 0;    
}

u16 F_Read(void *dest, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_RDONLY))
    {
        if (file->f.flags & FM_IO)
        {
            // ... read from io here
            kprintf("Attempting to read from IO file - Not implemented.");
        }
        else 
        {
            return FS_ReadFile(dest, size*count, &file->f, file->fname);
        }
    }

    return 0;    
}

u16 F_Write(void *src, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_WRONLY))
    {
        if (file->f.flags & FM_IO)
        {
            // ... write to io here
            kprintf("Attempting to write to IO file - Not implemented.");
        }
        else 
        {
            return FS_WriteFile(src, size*count, &file->f, file->fname);
        }
    }

    return 0;    
}

s8 F_Seek(SM_File *file, s32 offset, FileOrigin origin)
{
    return FS_Seek(&file->f, offset, origin, file->fname);
}

u16 F_Tell(SM_File *file)
{
    return FS_Tell(&file->f, file->fname);
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
