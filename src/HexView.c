
#include "HexView.h"
#include "Input.h"
#include "UI.h"
#include "Buffer.h"
#include "Utils.h"

static u32 FileOffset = 0;
static s16 ScrollY = 0;
bool bShowHexView = FALSE;
SM_Window HexWindow;

void DrawDataLine(u8 Line)
{
    char buf[41];
    sprintf(buf, "%04lX %02X %02X %02X %02X %02X %02X %02X %02X %c%c%c%c%c%c%c%c", 
    FileOffset, RxBuffer.data[FileOffset+0], RxBuffer.data[FileOffset+1], RxBuffer.data[FileOffset+2], RxBuffer.data[FileOffset+3], RxBuffer.data[FileOffset+4], RxBuffer.data[FileOffset+5], RxBuffer.data[FileOffset+6], RxBuffer.data[FileOffset+7],
    (char)((RxBuffer.data[FileOffset+0] >= 0x20) && (RxBuffer.data[FileOffset+0] <= 0x7E) ? RxBuffer.data[FileOffset+0] : '.'), 
    (char)((RxBuffer.data[FileOffset+1] >= 0x20) && (RxBuffer.data[FileOffset+1] <= 0x7E) ? RxBuffer.data[FileOffset+1] : '.'), 
    (char)((RxBuffer.data[FileOffset+2] >= 0x20) && (RxBuffer.data[FileOffset+2] <= 0x7E) ? RxBuffer.data[FileOffset+2] : '.'), 
    (char)((RxBuffer.data[FileOffset+3] >= 0x20) && (RxBuffer.data[FileOffset+3] <= 0x7E) ? RxBuffer.data[FileOffset+3] : '.'), 
    (char)((RxBuffer.data[FileOffset+4] >= 0x20) && (RxBuffer.data[FileOffset+4] <= 0x7E) ? RxBuffer.data[FileOffset+4] : '.'), 
    (char)((RxBuffer.data[FileOffset+5] >= 0x20) && (RxBuffer.data[FileOffset+5] <= 0x7E) ? RxBuffer.data[FileOffset+5] : '.'), 
    (char)((RxBuffer.data[FileOffset+6] >= 0x20) && (RxBuffer.data[FileOffset+6] <= 0x7E) ? RxBuffer.data[FileOffset+6] : '.'), 
    (char)((RxBuffer.data[FileOffset+7] >= 0x20) && (RxBuffer.data[FileOffset+7] <= 0x7E) ? RxBuffer.data[FileOffset+7] : '.'));

    UI_DrawText(0, Line, buf);
}

void UpdateView()
{
    u16 p = ScrollY << 3;
    UI_Begin(&HexWindow);

    FileOffset = p;

    for (u8 l = 0; l < 24; l++) 
    {
        DrawDataLine(l);
        FileOffset += 8;
    }

    UI_DrawVLine(4, 0, 24, UC_VLINE_SINGLE);
    UI_DrawVLine(28, 0, 24, UC_VLINE_SINGLE);
    UI_DrawVScrollbar(37, 0, 24, 0, (BUFFER_LEN-1)-0xC0+2, p);   // 0xFFF = RxBuf size - 0xC0 = amount of data on a single screen + 2 to make sure slider doesn't go over down arrow

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
        if (ScrollY < ((BUFFER_LEN/8)-24))//296) 
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
        if (ScrollY < ((BUFFER_LEN/8)-31))//289) 
        {
            ScrollY += 8;
            UpdateView();
        }
        else if (ScrollY < ((BUFFER_LEN/8)-24))
        {
            ScrollY += ((BUFFER_LEN/8)-24)-ScrollY;
            UpdateView();
        }
    }

    if (is_KeyDown(KEY_ESCAPE))
    {
        HexView_Toggle();
    }

    return;
}

void DrawHexView()
{
    VDP_setWindowVPos(FALSE, 30);
    TRM_clearTextArea(0, 0, 35, 1);
    TRM_clearTextArea(0, 1, 40, 29);

    UI_CreateWindow(&HexWindow, "HexView - Rx Buffer");

    FileOffset = 0;
    ScrollY = 0;

    UpdateView();
}

void HexView_Toggle()
{
    if (bShowHexView)
    {
        VDP_setWindowVPos(FALSE, 1);
        TRM_clearTextArea(0, 0, 36, 1);
        TRM_drawText(STATUS_TEXT, 1, 0, PAL1);
    }
    else
    {
        DrawHexView();
    }

    bShowHexView = !bShowHexView;
}
