#include "HexView.h"
#include "Input.h"
#include "UI.h"
#include "Network.h"
#include "Utils.h"
#include "system/Stdout.h"

static u32 FileOffset = 0;
static s16 ScrollY = 0;
static SM_Window HexWindow;
static Buffer *bufptr = NULL;
static char WinTitle[16];
bool bShowHexView = FALSE;


void DrawDataLine(u8 Line)
{
    char buf[41];
    sprintf(buf, "%04lX %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c", 
    FileOffset, bufptr->data[FileOffset+0], bufptr->data[FileOffset+1], bufptr->data[FileOffset+2], bufptr->data[FileOffset+3], bufptr->data[FileOffset+4], bufptr->data[FileOffset+5], bufptr->data[FileOffset+6], bufptr->data[FileOffset+7],
    (char)((bufptr->data[FileOffset+0] >= 0x20) && (bufptr->data[FileOffset+0] <= 0x7E) ? bufptr->data[FileOffset+0] : '.'), 
    (char)((bufptr->data[FileOffset+1] >= 0x20) && (bufptr->data[FileOffset+1] <= 0x7E) ? bufptr->data[FileOffset+1] : '.'), 
    (char)((bufptr->data[FileOffset+2] >= 0x20) && (bufptr->data[FileOffset+2] <= 0x7E) ? bufptr->data[FileOffset+2] : '.'), 
    (char)((bufptr->data[FileOffset+3] >= 0x20) && (bufptr->data[FileOffset+3] <= 0x7E) ? bufptr->data[FileOffset+3] : '.'), 
    (char)((bufptr->data[FileOffset+4] >= 0x20) && (bufptr->data[FileOffset+4] <= 0x7E) ? bufptr->data[FileOffset+4] : '.'), 
    (char)((bufptr->data[FileOffset+5] >= 0x20) && (bufptr->data[FileOffset+5] <= 0x7E) ? bufptr->data[FileOffset+5] : '.'), 
    (char)((bufptr->data[FileOffset+6] >= 0x20) && (bufptr->data[FileOffset+6] <= 0x7E) ? bufptr->data[FileOffset+6] : '.'), 
    (char)((bufptr->data[FileOffset+7] >= 0x20) && (bufptr->data[FileOffset+7] <= 0x7E) ? bufptr->data[FileOffset+7] : '.'));

    UI_DrawText(0, Line, PAL1, buf);
}

void UpdateView()
{
    u16 p = ScrollY << 3;
    FileOffset = p;

    UI_Begin(&HexWindow);

    for (u8 l = 0; l < 23; l++) 
    {
        DrawDataLine(l);
        FileOffset += 8;
    }

    UI_DrawVLine(4, 0, 23);
    UI_DrawVLine(28, 0, 23);
    UI_DrawVScrollbar(37, 0, 23, 0, (BUFFER_LEN-1)-0xB8+64, p);   // 0xFFF = Buffer size - 0xB8 = amount of data on a single screen + 64 to make sure slider doesn't go over down arrow

    UI_End();
}

void HexView_Input()
{
    if (!bShowHexView) return;

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
        if (ScrollY < ((BUFFER_LEN/8)-23)) 
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
        if (ScrollY < ((BUFFER_LEN/8)-31))
        {
            ScrollY += 8;
            UpdateView();
        }
        else if (ScrollY < ((BUFFER_LEN/8)-23))
        {
            ScrollY += ((BUFFER_LEN/8)-23)-ScrollY;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        HexView_Toggle(0);
    }

    return;
}

void DrawHexView()
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 26, PAL1, TRM_CLEAR_BG);  // h=27

    UI_CreateWindow(&HexWindow, WinTitle, UC_NONE);

    UI_Begin(&HexWindow);
    UI_FillRect(0, 27, 40, 2, 0xDE);
    UI_End();

    FileOffset = 0;
    ScrollY = 0;

    UpdateView();
}

void HexView_Toggle(u8 bufnum)
{
    if (bShowHexView)
    {
        TRM_SetWinHeight(1);

        // Clear hex viewer window tiles
        TRM_ClearArea(0, 1, 40, 26 + (bPALSystem?2:0), PAL1, TRM_CLEAR_BG);
        
        // Erase bottom most row tiles (May obscure IRC text input box). 
        // Normally the entire window should be erased by this call, but not all other windows may fill in the erased (black opaque) tiles.
        TRM_ClearArea(0, 27 + (bPALSystem?2:0), 40, 1, PAL1, TRM_CLEAR_INVISIBLE);
    }
    else
    {
        switch (bufnum)
        {
            case 0:
                strcpy(WinTitle, "HexView - Rx Buffer");
                bufptr = &RxBuffer;
            break;
            case 1:
                strcpy(WinTitle, "HexView - Tx Buffer");
                bufptr = &TxBuffer;
            break;
            case 2:
                strcpy(WinTitle, "HexView - Stdout");
                bufptr = &stdout;
            break;
        
            default:
            break;
        }
        
        DrawHexView();
    }

    bShowHexView = !bShowHexView;
}
