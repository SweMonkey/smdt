#include "SRAM.h"
#include "Utils.h"              // For EMU_BUILD define
#include "misc/VarList.h"

#define SAVE_VERSION 2

static u32 Offset = 0;


void SRAM_ClearSRAM()
{
    vu32 *SStart = (u32*)0x1B4;
    vu32 *SEnd = (u32*)0x1B8;
    u16 SSize = (*SEnd-*SStart) >> 1;
    //kprintf("SRAM Size: $%X", SSize);

    #ifdef KERNEL_BUILD
    return;
    #endif

    SRAM_enable();

    for (u16 i = 0; i < SSize; i++)
    {
        SRAM_writeByte(i, 0);
    }

    SRAM_disable();
}

void WriteByte(u8 *b)
{
    SRAM_writeByte(Offset, *b);
    Offset += 1;
}

void WriteWord(u16 *w)
{
    SRAM_writeWord(Offset, *w);
    Offset += 2;
}

void WriteLong(u32 *l)
{
    SRAM_writeLong(Offset, *l);
    Offset += 4;
}

void WriteStringPtr(char **str)
{
    char *s = *str;
    u16 len = strlen(s);
    for (u16 i = 0; i < len; i++)
    {
        SRAM_writeByte(Offset+i, s[i]);
    }
    
    SRAM_writeByte(Offset+len, 0);

    Offset += len+1;
}

void WriteCharArr(char *str)
{
    u16 len = strlen(str);
    for (u16 i = 0; i < len; i++)
    {
        SRAM_writeByte(Offset+i, str[i]);
    }
    
    SRAM_writeByte(Offset+len, 0);

    Offset += len+1;
}

void SRAM_SaveData()
{
    u16 i = 0;
    u8 bBreak = FALSE;

    #ifdef KERNEL_BUILD
    return;
    #endif

    SRAM_ClearSRAM();

    SRAM_enable();

    Offset = 0;

    SRAM_writeByte(Offset++, 'S');
    SRAM_writeByte(Offset++, 'M');
    SRAM_writeByte(Offset++, 'D');
    SRAM_writeByte(Offset++, 'T');
    SRAM_writeWord(Offset, SAVE_VERSION); Offset += 2;

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

    SRAM_disable();
}

void ReadByte(u8 *b)
{
    *b = SRAM_readByte(Offset);
    Offset += 1;
}

void ReadWord(u16 *w)
{
    *w = SRAM_readWord(Offset);
    Offset += 2;
}

void ReadLong(u32 *l)
{
    *l = SRAM_readLong(Offset);
    Offset += 4;
}

void ReadStringPtr(char **str)
{
    char *s = *str;
    char c;
    u16 len = 0;

    for (u16 i = 0; i < 1024; i++)
    {
        c = (char)SRAM_readByte(Offset+i);

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
        c = (char)SRAM_readByte(Offset+i);

        str[i] = c;

        if (c == 0)
        {
            len = i;
            break;
        }
    }

    Offset += len+1;
}

u8 SRAM_LoadData()
{
    u16 i = 0;
    u8 bBreak = FALSE;
    char magic[5];
    u16 version;

    #ifdef KERNEL_BUILD
    return 0;
    #endif

    SRAM_enableRO();

    Offset = 0;

    magic[0] = SRAM_readByte(Offset++);
    magic[1] = SRAM_readByte(Offset++);
    magic[2] = SRAM_readByte(Offset++);
    magic[3] = SRAM_readByte(Offset++);
    magic[4] = 0;
    
    version = SRAM_readWord(Offset); Offset += 2;

    if ((strcmp("SMDT", magic) != 0) || (version != SAVE_VERSION))
    {
        #ifdef EMU_BUILD
        kprintf("Save is invalid; Magic = \"%s\" - Version: %u", magic, version);
        #endif

        SRAM_disable();
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

    SRAM_disable();
    return 0;
}
