#include "HexView.h"
#include "Input.h"
#include "UI.h"
#include "Network.h"
#include "Utils.h"
#include "system/Stdout.h"

bool bShowHexView = FALSE;
static u16 FileOffset = 0;
static s16 ScrollY = 0;
static SM_Window *HexWindow = NULL;
static char *bufptr = NULL;
static u16 bufsize = 0;
static char WinTitle[38];
static bool bIOFILE = FALSE;


static void DrawDataLine(u8 Line)
{
    char buf[41];
    sprintf(buf, "%04X %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c", 
    (u16)FileOffset, 
    FileOffset+0 < bufsize ? (u8)bufptr[FileOffset+0] : 0, 
    FileOffset+1 < bufsize ? (u8)bufptr[FileOffset+1] : 0, 
    FileOffset+2 < bufsize ? (u8)bufptr[FileOffset+2] : 0, 
    FileOffset+3 < bufsize ? (u8)bufptr[FileOffset+3] : 0, 
    FileOffset+4 < bufsize ? (u8)bufptr[FileOffset+4] : 0, 
    FileOffset+5 < bufsize ? (u8)bufptr[FileOffset+5] : 0, 
    FileOffset+6 < bufsize ? (u8)bufptr[FileOffset+6] : 0, 
    FileOffset+7 < bufsize ? (u8)bufptr[FileOffset+7] : 0,
    (char)(((bufptr[FileOffset+0] >= 0x20) && (bufptr[FileOffset+0] <= 0x7E) && (FileOffset+0 < bufsize)) ? (u8)bufptr[FileOffset+0] : '.'), 
    (char)(((bufptr[FileOffset+1] >= 0x20) && (bufptr[FileOffset+1] <= 0x7E) && (FileOffset+1 < bufsize)) ? (u8)bufptr[FileOffset+1] : '.'), 
    (char)(((bufptr[FileOffset+2] >= 0x20) && (bufptr[FileOffset+2] <= 0x7E) && (FileOffset+2 < bufsize)) ? (u8)bufptr[FileOffset+2] : '.'), 
    (char)(((bufptr[FileOffset+3] >= 0x20) && (bufptr[FileOffset+3] <= 0x7E) && (FileOffset+3 < bufsize)) ? (u8)bufptr[FileOffset+3] : '.'), 
    (char)(((bufptr[FileOffset+4] >= 0x20) && (bufptr[FileOffset+4] <= 0x7E) && (FileOffset+4 < bufsize)) ? (u8)bufptr[FileOffset+4] : '.'), 
    (char)(((bufptr[FileOffset+5] >= 0x20) && (bufptr[FileOffset+5] <= 0x7E) && (FileOffset+5 < bufsize)) ? (u8)bufptr[FileOffset+5] : '.'), 
    (char)(((bufptr[FileOffset+6] >= 0x20) && (bufptr[FileOffset+6] <= 0x7E) && (FileOffset+6 < bufsize)) ? (u8)bufptr[FileOffset+6] : '.'), 
    (char)(((bufptr[FileOffset+7] >= 0x20) && (bufptr[FileOffset+7] <= 0x7E) && (FileOffset+7 < bufsize)) ? (u8)bufptr[FileOffset+7] : '.'));

    UI_DrawText(0, Line, PAL1, buf);
}

static void UpdateView()
{
    u16 p = ScrollY << 3;
    FileOffset = p;

    UI_Begin(HexWindow);

    for (u8 l = 0; l < 23; l++) 
    {
        DrawDataLine(l);
        FileOffset += 8;
    }

    UI_DrawVLine(4, 0, 23);
    UI_DrawVLine(28, 0, 23);
    UI_DrawVScrollbar(37, 0, 23, 0, (bufsize-1)-0xB8+64, p);   // 0xFFF = Buffer size - 0xB8 = amount of data on a single screen + 64 to make sure slider doesn't go over down arrow

    UI_End();
}

void HexView_Input()
{
    if (!bShowHexView || (HexWindow == NULL)) return;

    if (is_KeyDown(KEY_UP))
    {
        if (ScrollY > 0) 
        {
            ScrollY--;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_DOWN))
    {
        if (ScrollY < ((bufsize/8)-23)) 
        {
            ScrollY++;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_LEFT))
    {
        if (ScrollY > 7) 
        {
            ScrollY -= 8;
            UpdateView();
        }
        else if (ScrollY > 0)
        {
            ScrollY = 0;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_RIGHT))
    {
        if (ScrollY < ((bufsize/8)-31))
        {
            ScrollY += 8;
            UpdateView();
        }
        else if (ScrollY < ((bufsize/8)-23))
        {
            ScrollY += ((bufsize/8)-23)-ScrollY;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        HexView_Close();
    }

    return;
}

static void DrawHexView()
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 26, PAL1, TRM_CLEAR_BG);  // h=27

    UI_CreateWindow(HexWindow, WinTitle, UC_NONE);

    UI_Begin(HexWindow);
    UI_FillRect(0, 27, 40, 2, 0xDE);
    UI_End();

    FileOffset = 0;
    ScrollY = 0;

    UpdateView();
}

void HexView_Open(const char *filename)
{
    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(filename, fn_buf);
    bIOFILE = FALSE;

    if (strcmp(fn_buf, "/system/rxbuffer.io") == 0)
    {
        strcpy(WinTitle, "HexView - Rx Buffer");
        
        bufptr = (char*)RxBuffer.data;
        bufsize = BUFFER_LEN;
        bIOFILE = TRUE;
    }
    else if (strcmp(fn_buf, "/system/txbuffer.io") == 0)
    {
        strcpy(WinTitle, "HexView - Tx Buffer");
        
        bufptr = (char*)TxBuffer.data;
        bufsize = BUFFER_LEN;
        bIOFILE = TRUE;
    }
    else if (strcmp(fn_buf, "/system/stdout.io") == 0)
    {
        strcpy(WinTitle, "HexView - Stdout");
        
        bufptr = (char*)StdoutBuffer.data;
        bufsize = BUFFER_LEN;
        bIOFILE = TRUE;
    }
    else
    {
        SM_File *f = F_Open(fn_buf, FM_RDONLY);
        if (f == NULL) 
        {
            printf("Failed to open file \"%s\"\n", filename);
            return;
        }

        F_Seek(f, 0, SEEK_END);
        bufsize = F_Tell(f);

        bufptr = (char*)malloc(bufsize+1);    
        if (bufptr == NULL)
        {
            printf("Failed to allocate buffer\n");
            F_Close(f);
            return;
        }

        memset(bufptr, 0, bufsize);
        F_Seek(f, 0, SEEK_SET);
        F_Read(bufptr, bufsize, 1, f);

        F_Close(f);

        snprintf(WinTitle, 38, "HexView - %s", filename);
    }

    HexWindow = malloc(sizeof(SM_Window));

    if (HexWindow == NULL)
    {
        Stdout_Push("[91mFailed to allocate memory;\nCan't create HexWindow[0m\n");
        return;
    }

    DrawHexView();

    bShowHexView = TRUE;
}

void HexView_Close()
{
    TRM_SetWinHeight(1);

    // Clear hex viewer window tiles
    TRM_ClearArea(0, 1, 40, 26 + (bPALSystem?2:0), PAL1, TRM_CLEAR_BG);
    
    // Erase bottom most row tiles (May obscure IRC text input box). 
    // Normally the entire window should be erased by this call, but not all other windows may fill in the erased (black opaque) tiles.
    TRM_ClearArea(0, 27 + (bPALSystem?2:0), 40, 1, PAL1, TRM_CLEAR_INVISIBLE);

    if (bIOFILE == FALSE)
    {
        free(bufptr);
        bufptr = NULL;
    }

    if (HexWindow != NULL)
    {
        free(HexWindow);
        HexWindow = NULL;
        MEM_pack();
    }

    bufsize = 0;
    bShowHexView = FALSE;
}
