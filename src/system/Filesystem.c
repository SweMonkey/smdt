// FS
#include "Filesystem.h"
#include "Utils.h"

#ifdef KERNEL_BUILD

asm(".global diskimg\ndiskimg:\n.incbin \"res/Default_disk2.img\"");
extern const unsigned char diskimg[];

#define FS_OFFSET 0//0x400

u8 sector[SECTOR_SIZE];
u32 pstart, psize, i;
u8 pactive, ptype;
VOLINFO vi;
DIRINFO di;
DIRENT de;

static bool bInitFail = FALSE;


u32 DFS_ReadSector(u8 unit, u8 *buffer, u32 sector, u32 count)
{
    /*if (bInitFail)
    {
        kprintf("ReadSector: bInitFail = TRUE");
        return 1;
    }*/

    u32 i = 0;

    SRAM_enableRO();

    kprintf("Sector read from: $%lX to $%lX", FS_OFFSET + (sector * SECTOR_SIZE), (FS_OFFSET + (sector * SECTOR_SIZE)) + ((SECTOR_SIZE+count)-1));

    if ((FS_OFFSET + (sector * SECTOR_SIZE) >= 0x20000) || ((FS_OFFSET + (sector * SECTOR_SIZE)) + ((SECTOR_SIZE+count)-1) >= 0x20000)) return 1;

    while ((i < (SECTOR_SIZE+count-1)) )// && (i < 0x1FFFF))
    {
        buffer[i] = SRAM_readByte(FS_OFFSET + (sector * SECTOR_SIZE) + i);
        i++;
    }

    SRAM_disable();

    return 0;
}

u32 DFS_WriteSector(u8 unit, u8 *buffer, u32 sector, u32 count)
{
    /*if (bInitFail)
    {
        kprintf("WriteSector: bInitFail = TRUE");
        return 1;
    }*/

    u32 i = 0;

    SRAM_enable();

    //kprintf("Sector write sector: $%lu - count: $%lu", sector, count);
    //kprintf("Sector write from: $%lX to $%lX", FS_OFFSET + (sector * SECTOR_SIZE), 
    //                                          (FS_OFFSET + (sector * SECTOR_SIZE)) + ((SECTOR_SIZE+count)-1));
                                              
    if ((FS_OFFSET + (sector * SECTOR_SIZE) >= 0x20000) || ((FS_OFFSET + (sector * SECTOR_SIZE)) + ((SECTOR_SIZE+count)-1) >= 0x20000)) return 1;

    while ((i < (SECTOR_SIZE+count-1)) )// && (i < 0x1FFFF))
    {
        SRAM_writeByte(FS_OFFSET + (sector * SECTOR_SIZE) + i, buffer[i]);

        i++;
    }

    //kprintf("Writing \"%s\" to $%lX -> $%lX", (char*)buffer, FS_OFFSET + (sector * SECTOR_SIZE), (FS_OFFSET + (sector * SECTOR_SIZE)) + ((SECTOR_SIZE+count)-1));

    SRAM_disable();

    return 0;
}

void FS_Init()
{
    bInitFail = FALSE;

    //kprintf("Calling FS_Init()");

    SRAM_enableRO();
    u16 dskchk = SRAM_readWord(FS_OFFSET + 0x1FE);//0x55AA; // = SRAM @ 0x10..
    SRAM_disable();
    
    // Temp - always write a blank disk image to SRAM on init
    /*SRAM_enable();
    for (u16 i = 0; i < 7800; i++)
    {
        SRAM_writeByte(FS_OFFSET + i, diskimg[i]);
    }
    SRAM_disable();*/
    // Temp

    //Retry:

    if (dskchk != 0x55AA)  // 55AA
    {
        stdout_printf("Disk error! Signature check failed (r = $%X)\n", dskchk);
        //kprintf("Disk error! Signature check failed (r = $%X)", dskchk);

        SRAM_enable();
        for (u32 i = 0; i < 0x20000; i++)
        {
            SRAM_writeByte(FS_OFFSET + i, diskimg[i]);
        }

        //kprintf("Written new disk image... (r = $%X)", SRAM_readWord(FS_OFFSET + 0x1FE));

        SRAM_disable();

        //goto Retry;
    }

    pstart = DFS_GetPtnStart(0, sector, 0, &pactive, &ptype, &psize);

    if (pstart == DFS_ERRMISC)
    {
        stdout_printf("Cannot find first partition\n");
        //kprintf("Cannot find first partition");
        bInitFail = TRUE;
    }
    else if (DFS_GetVolInfo(0, sector, pstart, &vi))
    {
        stdout_printf("Error getting volume information\n");
        //kprintf("Error getting volume information");
        bInitFail = TRUE;
    }

    return;
}

void FS_PrintVolInfo()
{
    if (bInitFail) return;

    stdout_printf("Volume label: %s", vi.label);

    stdout_printf("\nSector/s per cluster: %u", vi.secperclus);
    stdout_printf("\nReserved sector/s: %u", vi.reservedsecs);
    stdout_printf("\nVolume total sectors: %lu", vi.numsecs);

    stdout_printf("\nSectors per FAT: %lu", vi.secperfat);
    stdout_printf("\nFirst FAT at sector: %lu", vi.fat1);
    stdout_printf("\nRoot dir at: %lu", vi.rootdir);

    stdout_printf("\nRoot dir entries: %u", vi.rootentries);
    stdout_printf("\nData area commences at sector: %lu", vi.dataarea);

    stdout_printf("\nClusters: %lu", vi.numclusters);
    stdout_printf("\nBytes: %lu", vi.numclusters * vi.secperclus * SECTOR_SIZE);

    stdout_printf("\nFilesystem IDd as: ");

    if (vi.filesystem == FAT12)
        stdout_printf("FAT12.\n");
    else if (vi.filesystem == FAT16)
        stdout_printf("FAT16.\n");
    else if (vi.filesystem == FAT32)
        stdout_printf("FAT32.\n");
    else
        stdout_printf("[unknown]\n");
}

void FS_ListDir(char *dir)
{
    if (bInitFail) return;

    di.scratch = sector;

    if (DFS_OpenDir(&vi, (u8*)dir, &di))
    {
        stdout_printf("Error opening directory \"%s\"\n", dir);
        return;
    }

    stdout_printf("Directory listing of %s\n", dir);

    u16 filecnt = 0;     // Num files in directory
    u32 fileszcnt = 0;   // Size of all files in directory
    u16 dircnt = 0;      // Num of directories
    u32 fsz = 0;         // Temporary individual file count
    char name[12];       // Formatted entry name
    char attr[16];       // Formatted entry attributes

    while (!DFS_GetNext(&vi, &di, &de))
    {
        if (de.name[0])
        {
            memset(name, '\0', 12);

            u8 nlen = strlen((const char*)de.name);

            strncpy(name, (const char*)de.name, (nlen > 11) ? 11 : nlen);

            switch (de.attr)
            {
                case ATTR_READ_ONLY:
                    strncpy(attr, "<RDO>", 16);
                break;

                case ATTR_HIDDEN:
                    strncpy(attr, "<HID>", 16);
                break;

                case ATTR_SYSTEM:
                    strncpy(attr, "<SYS>", 16);
                break;

                case ATTR_VOLUME_ID:
                    strncpy(attr, "<VOL>", 16);
                break;

                case ATTR_DIRECTORY:
                    strncpy(attr, "<DIR>", 16);
                break;

                case ATTR_ARCHIVE:
                    strncpy(attr, "<BIN>", 16);
                break;

                case ATTR_LONG_NAME:
                    strncpy(attr, "<LFN>", 16);
                break;

                default:
                    strncpy(attr, "<UNK>", 16);
                break;
            }

            u32 wrtd = (de.wrtdate_h << 8) | (de.wrtdate_l);

            u16 y = ((wrtd & 0xFE00) >> 9)+1980;
            u16 M = (wrtd & 0x1E0) >> 5;
            u16 d = (wrtd & 0x1F);

            u32 wrtt = (de.wrttime_h << 8) | (de.wrttime_l);

            u16 h = ((wrtt & 0xF800) >> 11);
            u16 m = (wrtt & 0x7E0) >> 5;
            u16 s = (wrtt & 0x1F) * 2;

            if ((de.attr & ATTR_DIRECTORY) || (de.attr & ATTR_VOLUME_ID))
            {
                 stdout_printf("%u-%02u-%02u %02u:%02u:%02u            %s %s\n", y, M, d, h, m, s, attr, name);

                 if (((de.attr & ATTR_VOLUME_ID) == 0) && (name[0] != '.')) dircnt++;
            }
            else
            {
                fsz = ((de.filesize_3 << 24) | (de.filesize_2 << 16) | (de.filesize_1 << 8) | de.filesize_0);
                stdout_printf("%u-%02u-%02u %02u:%02u:%02u %10lu %s %s\n", y, M, d, h, m, s, fsz, attr, name);
                filecnt++;
                fileszcnt += fsz;
            }
        }
    }

    stdout_printf("\n%7u file%s           %10lu bytes\n", filecnt, ((filecnt == 1) ? " " : "s"), fileszcnt);
    stdout_printf("%7u director%s      %11lu bytes total\n", dircnt, ((dircnt == 1) ? "y" : "ies"), vi.numclusters * vi.secperclus * SECTOR_SIZE); // Todo swap total to free bytes
}

u32 FS_OpenFile(const char *filename, u8 openmode, PFILEINFO fi)
{
    return DFS_OpenFile(&vi, (u8*)filename, openmode, sector, fi);
}

u32 FS_ReadFile(PFILEINFO fi, void *dest, u32 len)
{
    u32 r = DFS_ReadFile(fi, sector, (u8*)dest, &i, len);

    switch (r)
    {
        case DFS_ERRMISC:
        case DFS_OK:
        case DFS_EOF:
        default:
        break;
    }

    return i;
}

u32 FS_WriteFile(PFILEINFO fi, void *src, u32 len)
{
    u32 r = DFS_WriteFile(fi, sector, src, &i, len);

    switch (r)
    {
        case DFS_ERRMISC:
        case DFS_OK:
        case DFS_EOF:
        default:
        break;
    }

    return i;
}

u8 FS_Unlink(const char *filename)
{
    if (DFS_UnlinkFile(&vi, (u8*)filename, sector)) 
    {
        stdout_printf("Error unlinking file \"%s\"\n", filename);
        return 1;
    }

    return 0;
}

#endif // KERNEL_BUILD
