#include "File.h"
#include "Filesystem.h"
#include "Utils.h"

#ifdef KERNEL_BUILD

#define MAX_FD 64

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
        stdout_printf("No free filedescriptors available!\n");
        return NULL;
    }

    u32 r = FS_OpenFile(filename, openmode, &file->fi);

    if (r)
    {
        stdout_printf("Error opening file \"%s\"\nError code $%lX\n", filename, r);

        return NULL;
    }

    //stdout_printf("Opened file \"%s\"\n", filename);

    return file;
}

u8 F_Close(SM_File *file)
{
    if (file == NULL) 
    {
        stdout_printf("Error closing file\n");
        return -1; // EOF
    }

    free(file);
    file = NULL;

    FD_Count--;

    return 0;    
}

u16 F_Read(void *dest, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if (file->fi.mode == FM_READ)
    {
        return FS_ReadFile(&file->fi, dest, size*count);;
    }

    return 0;    
}

u16 F_Write(void *src, u32 size, u32 count, SM_File *file)
{
    if (file == NULL) return 0;

    if (file->fi.mode == FM_WRITE)
    {
        return FS_WriteFile(&file->fi, src, size*count);;
    }

    return 0;    
}

s8 F_Seek(SM_File *file, s32 offset, FileOrigin origin)
{
    switch (origin)
    {
        case SEEK_SET:
        {
            file->fi.pointer = offset;
            return 0;
        }
        case SEEK_CUR:
        {
            file->fi.pointer += offset;
            return 0;
        }
        case SEEK_END:
        {
            file->fi.pointer = file->fi.filelen;
            return 0;
        }
    }

    return -1;
}

u16 F_Tell(SM_File *file)
{
    return file->fi.pointer;
}

#endif // KERNEL_BUILD
