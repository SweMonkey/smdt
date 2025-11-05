#include "File.h"
#include "Filesystem.h"
#include "Utils.h"

#include "system/PseudoFile.h"  // Temp, printf

#define MAX_FD 64
#define MAX_PRINTF_BUF 256

static u16 FD_Count = 0;    // Open file descriptors - Not used for anything other than limiting the user/os to MAX_FD number of open files at once 

extern SM_File *FILE_INTERNAL_tty_in;
extern SM_File *FILE_INTERNAL_tty_out;
extern SM_File *FILE_INTERNAL_stdin;
extern SM_File *FILE_INTERNAL_stdout;
extern SM_File *FILE_INTERNAL_stderr;


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
    
    u16 i = 0;
    u16 p = 0;
    while (filename[i] != '\0')
    {
        if (filename[i] == '/') p = i;
        i++;
    }
    p++;    

    // Map special I/O pseudo-files
    if ((strcmp(filename+p, "stdin.io")   == 0) && (FILE_INTERNAL_stdin   != NULL)) return FILE_INTERNAL_stdin;
    if ((strcmp(filename+p, "stdout.io")  == 0) && (FILE_INTERNAL_stdout  != NULL)) return FILE_INTERNAL_stdout;
    if ((strcmp(filename+p, "stderr.io")  == 0) && (FILE_INTERNAL_stderr  != NULL)) return FILE_INTERNAL_stderr;
    if ((strcmp(filename+p, "tty_in.io")  == 0) && (FILE_INTERNAL_tty_in  != NULL)) return FILE_INTERNAL_tty_in;
    if ((strcmp(filename+p, "tty_out.io") == 0) && (FILE_INTERNAL_tty_out != NULL)) return FILE_INTERNAL_tty_out;

    s32 r = FS_OpenFile(filename, openmode, &file->f);

    if (r)
    {
        F_Close(file);  // Close file to avoid leaking memory...
        //printf("Error opening file \"%s\"\nError code $%lX (%ld)\n", filename, r, r);
        return NULL;
    }

    file->io_buf = NULL;

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

    // Dont unmap I/O pseudo-files
    if (file->f.flags & FM_IO) 
    {
        //kprintf("Attempted to unmap IO file (%s)", file == FILE_INTERNAL_stdout ? "internal_stdout" : "other");
        return 0;
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
    if (file == NULL || !size || !count) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_RDONLY))
    {
        if (file->f.flags & FM_IO && file->io_buf != NULL)
        {
            //kprintf("Read from IO file");

            Buffer *b = file->io_buf;

            u8 *data = (u8 *)dest;
            u32 total = size * count;
            u32 read = 0;

            if (dest == NULL) return 0;

            for (u32 i = 0; i < total; i++)
            {
                if (!Buffer_Pop(b, &data[i])) break;  // stop if empty
                
                read++;
            }

            return read / size;
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
    if (file == NULL || !size || !count) return 0;

    if ((file->f.flags & FM_RDWR) || (file->f.flags & FM_WRONLY))
    {
        if (file->f.flags & FM_IO && file->io_buf != NULL)
        {
            Buffer *b = file->io_buf;

            const u8 *data = (const u8 *)src;
            u32 total = size * count;
            u32 written = 0;

            for (u32 i = 0; i < total; i++)
            {
                if (!Buffer_Push(b, data[i])) 
                {
                    Stdout_Flush();                    
                    break;  // stop if full
                }

                written++;
            }

            return written / size;
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
