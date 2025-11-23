#include "HexView.h"
#include "Input.h"
#include "UI.h"
#include "Network.h"
#include "Utils.h"
#include "WinMgr.h"
#include "Mouse.h"          // MHitRect
#include "system/PseudoFile.h"

static u16 FileOffset = 0;
static s16 ScrollY = 0;
static SM_Window *HexWindow = NULL;
static char *bufptr = NULL;
static u16 bufsize = 0;
static char WinTitle[38];
static bool bIOFILE = FALSE;
static u8 NumLines = 23;
static u16 ScrollMax = 0;
static u16 r = 255;         // Mouse widget collision target

static const MRect mrect_data[] =
{
    {304,  32, 8,   8, 0},   // Up
    {304,  40, 8, 168, 1},   // Slider area
    {304, 208, 8,   8, 2},   // Down
    {320,   0, 0,   0, 0},   // Terminator
};


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
    FileOffset = ScrollY;

    UI_Begin(HexWindow);

    for (u8 l = 0; l < NumLines; l++) 
    {
        DrawDataLine(l);
        FileOffset += 8;
    }

    UI_DrawVLine(4, 0, 23);
    UI_DrawVLine(28, 0, 23);
    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);

    UI_End();
}

void HexView_Input()
{
    if (bMouse)
    {
        r = Mouse_GetRect(mrect_data) & 0x7F;

        switch (r)
        {
            case 0: // Up
                if (is_KeyDown(sv_MBind_Click))
                {
                    if (ScrollY > 7) 
                    {
                        ScrollY -= 8;
                        UpdateView();
                    }
                }
                else // Hover
                {
                    UI_Begin(HexWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;

            case 1: // Slider     
                if (is_KeyDown(sv_MBind_Click))
                {
                    // ...
                }
                else // Hover
                {
                    UI_Begin(HexWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }       
            break;

            case 2: // Down
                if (is_KeyDown(sv_MBind_Click))
                {
                    if (ScrollY < (bufsize-184))
                    {
                        ScrollY += 8;
                        UpdateView();
                    }
                }
                else // Hover
                {
                    UI_Begin(HexWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;
        
            default:
                if (is_KeyDown(sv_MBind_Click) == FALSE)
                {
                    UI_Begin(HexWindow);                    
                    UI_DrawVScrollbar(37, 0, 23, r, 0, ScrollMax, ScrollY>>3);
                    UI_RepaintColumn(38, 1);
                    UI_EndNoPaint();
                }
            break;
        }
    }

    if (is_KeyDown(KEY_UP))
    {
        if (ScrollY > 7) 
        {
            ScrollY -= 8;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_DOWN))
    {
        if (ScrollY < (bufsize-184))
        {
            ScrollY += 8;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_LEFT))
    {
        if (ScrollY > 183) 
        {
            ScrollY -= 184;
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
        if (ScrollY < (bufsize-368))
        {
            ScrollY += 184;
            UpdateView();
        }
        else if (ScrollY < (bufsize-184))
        {
            ScrollY += (bufsize-184)-ScrollY;

            u8 rem = ScrollY % 8;
            if (rem != 0) ScrollY += 8-rem;

            UpdateView();
        }
    }

    if (is_KeyUp(KEY_ESCAPE) || is_KeyUp(sv_MBind_AltClick))
    {
        WinMgr_Close(W_HexView);
    }

    return;
}

static void DrawHexView()
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 26, PAL1, TRM_CLEAR_BG);  // h=27

    UI_CreateWindow(HexWindow, WinTitle, WF_None);

    UI_Begin(HexWindow);
    UI_FillRect(0, 27, 40, 2, 0xDE);
    UI_End();

    FileOffset = 0;
    ScrollY = 0;

    UpdateView();
}

u16 HexView_Open(const char *filename)
{
    char fn_buf[FILE_MAX_FNBUF];
    FS_ResolvePath(filename, fn_buf);
    bIOFILE = FALSE;

    // Open file, take special care of IO files
    if (strcmp(fn_buf, "/sram/system/tty_in.io") == 0)
    {
        strcpy(WinTitle, "HexView - Rx Buffer");
        
        bufptr = (char*)RxBuffer.data;
        bufsize = BUFFER_LEN;
        bIOFILE = TRUE;
    }
    else if (strcmp(fn_buf, "/sram/system/tty_out.io") == 0)
    {
        strcpy(WinTitle, "HexView - Tx Buffer");
        
        bufptr = (char*)TxBuffer.data;
        bufsize = BUFFER_LEN;
        bIOFILE = TRUE;
    }
    else if (strcmp(fn_buf, "/sram/system/stdout.io") == 0)
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
            return 1;
        }

        F_Seek(f, 0, SEEK_END);
        bufsize = F_Tell(f);

        bufptr = (char*)malloc(bufsize+1);    
        if (bufptr == NULL)
        {
            printf("Failed to allocate buffer (Out of memory)\n");
            F_Close(f);
            return 1;
        }

        memset(bufptr, 0, bufsize);
        F_Seek(f, 0, SEEK_SET);
        F_Read(bufptr, bufsize, 1, f);

        F_Close(f);

        snprintf(WinTitle, 38, "HexView - %s", filename);
    }

    // Create hex viewer window
    HexWindow = malloc(sizeof(SM_Window));

    if (HexWindow == NULL)
    {
        printf("[91mFailed to allocate memory;\nCan't create HexWindow[0m\n");
        
        if (bIOFILE == FALSE)
        {
            free(bufptr);
            bufptr = NULL;
        }

        return 1;
    }

    // Determine number of lines that will be needed if file is smaller than 192 bytes
    if (bufsize < 0xC0)
    {
        NumLines  = bufsize / 8;
        NumLines += (bufsize % 8 ? 1 : 0);
        ScrollMax = 0;
    }
    else 
    {
        NumLines = 23;
        ScrollMax = ((bufsize)-0xB8) >> 3;  // 0xB8 = amount of data on a single screen
    }

    // Redraw hex viewer window and present it
    DrawHexView();

    return 0;
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
}
