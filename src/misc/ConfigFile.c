#include "ConfigFile.h"
#include "Utils.h"              // For EMU_BUILD define
#include "misc/VarList.h"
#include "system/File.h"

static const u8 CFG_Magic[5] = {'S', 'M', 'D', 'T', 0};
static const u16 CFG_Version = 3;

static u32 Offset = 0;
static u8 *cfgbuf = NULL;


void CFG_ClearConfig()
{
}

void WriteByte(const u8 *b)
{
    cfgbuf[Offset] = *b;
    Offset += 1;
}

void WriteWord(const u16 *w)
{
    cfgbuf[Offset++] = *w >> 8;
    cfgbuf[Offset++] = *w & 0xFF;
}

void WriteLong(const u32 *l)
{
    cfgbuf[Offset++] = (*l >> 24) & 0xFF;
    cfgbuf[Offset++] = (*l >> 16) & 0xFF;
    cfgbuf[Offset++] = (*l >> 8) & 0xFF;
    cfgbuf[Offset++] = *l & 0xFF;
}

void WriteStringPtr(char **str)
{
    char *s = *str;
    u16 len = strlen(s);
    for (u16 i = 0; i < len; i++)
    {
        cfgbuf[Offset+i] = s[i];
    }
    
    cfgbuf[Offset+len] = 0;

    Offset += len+1;
}

void WriteCharArr(char *str)
{
    u16 len = strlen(str);
    for (u16 i = 0; i < len; i++)
    {
        cfgbuf[Offset+i] = str[i];
    }
    
    cfgbuf[Offset+len] = 0;

    Offset += len+1;
}

void CFG_SaveData()
{
    u16 i = 0;
    u8 bBreak = FALSE;

    Offset = 0;
    
    SM_File *f = F_Open("/sram/system/smdt_cfg.bin", FM_CREATE | FM_WRONLY);

    if (f == NULL) return;

    cfgbuf = malloc(256);

    if (cfgbuf == NULL) 
    {
        F_Close(f);
        return;
    }

    WriteByte(&CFG_Magic[0]);
    WriteByte(&CFG_Magic[1]);
    WriteByte(&CFG_Magic[2]);
    WriteByte(&CFG_Magic[3]);
    WriteByte(&CFG_Magic[4]);
    WriteWord(&CFG_Version);
    
    while (!bBreak)
    {
        switch (VarList[i].size)
        {
            case 0:
                bBreak = TRUE;
            break;
            case ST_BYTE:
                WriteByte((u8*)VarList[i].ptr);
            break;
            case ST_WORD:
                WriteWord((u16*)VarList[i].ptr);
            break;
            case ST_LONG:
                WriteLong((u32*)VarList[i].ptr);
            break;        
            case ST_SPTR:
                WriteStringPtr((char**)VarList[i].ptr);
            break;
            case ST_SARR:
            default:
                WriteCharArr((char*)VarList[i].ptr);
            break;
        }

        i++;
    }

    F_Write(cfgbuf, Offset, 1, f);

    F_Close(f);
    free(cfgbuf);
    cfgbuf = NULL;

    return;
}

void ReadByte(u8 *b)
{
    *b = cfgbuf[Offset++];
}

void ReadWord(u16 *w)
{
    *w  = (cfgbuf[Offset++] << 8);
    *w |= (cfgbuf[Offset++]     );
}

void ReadLong(u32 *l)
{
    *l  = (cfgbuf[Offset++] << 24);
    *l |= (cfgbuf[Offset++] << 16);
    *l |= (cfgbuf[Offset++] <<  8);
    *l |= (cfgbuf[Offset++]      );
}

void ReadStringPtr(char **str)
{
    char *s = *str;
    char c;
    u16 len = 0;

    for (u16 i = 0; i < 1024; i++)
    {
        c = (char)cfgbuf[Offset+i];

        s[i] = c;

        if (c == 0)
        {
            len = i;
            break;
        }
    }

    Offset += len+1;
}

void ReadCharArr(char *str)
{
    char c;
    u16 len = 0;

    for (u16 i = 0; i < 1024; i++)
    {
        c = (char)cfgbuf[Offset+i];

        str[i] = c;

        if (c == 0)
        {
            len = i;
            break;
        }
    }

    Offset += len+1;
}

u8 CFG_LoadData()
{
    u16 i = 0;
    u8 bBreak = FALSE;
    u8 magic[5];
    u16 version;
    Offset = 0;

    SM_File *f = F_Open("/sram/system/smdt_cfg.bin", FM_RDONLY);
    if (f == NULL) return 1;

    F_Seek(f, 0, SEEK_END);
    u16 size = F_Tell(f);

    cfgbuf = (u8*)malloc(size);
    if (cfgbuf == NULL) 
    {
        F_Close(f);
        return 1;
    }

    memset(cfgbuf, 0, size);
    F_Seek(f, 0, SEEK_SET);
    F_Read(cfgbuf, size, 1, f);

    ReadByte(&magic[0]);
    ReadByte(&magic[1]);
    ReadByte(&magic[2]);
    ReadByte(&magic[3]);
    magic[4] = 0;
    Offset++;
    ReadWord(&version);

    if ((strcmp((char*)CFG_Magic, (char*)magic) != 0) || (version != CFG_Version))
    {
        #ifdef EMU_BUILD
        kprintf("Save is invalid; Magic = \"%s\" - Version: %u", magic, version);
        #endif
        
        F_Close(f);
        free(cfgbuf);
        cfgbuf = NULL;

        return 1;
    }

    while (!bBreak)
    {
        switch (VarList[i].size)
        {
            case 0:
                bBreak = TRUE;
            break;
            case ST_BYTE:
                ReadByte((u8*)VarList[i].ptr);
            break;
            case ST_WORD:
                ReadWord((u16*)VarList[i].ptr);
            break;
            case ST_LONG:
                ReadLong((u32*)VarList[i].ptr);
            break;        
            case ST_SPTR:
                ReadStringPtr((char**)VarList[i].ptr);
            break;
            case ST_SARR:
            default:
                ReadCharArr((char*)VarList[i].ptr);
            break;
        }

        i++;
    }

    F_Close(f);
    free(cfgbuf);
    cfgbuf = NULL;

    return 0;
}
