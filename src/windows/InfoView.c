#include "InfoView.h"
#include "Input.h"
#include "UI.h"
#include "Utils.h"
#include "system/PseudoFile.h"
#include "WinMgr.h"
#include "Network.h"
#include "Mouse.h"          // MHitRect
#include "system/Time.h"

static SM_Window *InfoWindow = NULL;
static s16 ScrollY = 0;
static s8 sIdx = 0;     // Selector idx between tab + buttons
static u16 tIdx = 0;    // Selector idx for tabs
static u32 UpdateCnt = 0;
static SM_Time uptime;
static u32 Lastdown = 0;
static u32 Lastup = 0;

static const MRect mrect_data[] =
{
    {  8,  32, 112, 8, 0},
    {136,  32,  56, 8, 1},
    {208,  32,  32, 8, 2},
    {255,   0,   0, 0, 0},   // Terminator
};

static const char * const tab_text[3] =
{
    "System Monitor", "Devices", "Info"
};

static void DrawSysMonTab()
{
    UI_DrawGroupBox(0,  2, 38, 5, "Resource usage");
    UI_DrawGroupBox(0,  8, 38, 5, "Buffer usage");
    UI_DrawGroupBox(0,  14, 20, 5, "Network traffic");
    UI_DrawGroupBox(20,  14, 18, 5, "Network speed");

    // - Resource usage -

    // CPU
    char cputext[64];
    u16 cpu = SYS_getCPULoad();
        cpu = (cpu > 100 ? 100 : cpu);
    snprintf(cputext, 38, "CPU                   %d %c", cpu, '%');

    UI_DrawText(1, 4, PAL1, cputext);
    UI_DrawHProgressBar(5, 4, 100, cpu);

    // RAM
    char ramtext[64];
    u16 ram = (65536 - MEM_getFree())/1024;
    u16 ram_max = 65536/1024;
    snprintf(ramtext, 38, "RAM                   %d/%d KiB", ram, ram_max);

    UI_DrawText(1, 5, PAL1, ramtext);
    UI_DrawHProgressBar(5, 5, ram_max, ram);

    // - Buffer usage -

    // RxBuffer
    char buftext[64];
    u16 rx_free = Buffer_GetNum(&RxBuffer);
    snprintf(buftext, 38, "Rx                   %u/%u bytes", rx_free, BUFFER_LEN);

    UI_DrawText(1, 10, PAL1, buftext);
    UI_DrawHProgressBar(4, 10, BUFFER_LEN/32, rx_free/32);

    // TxBuffer
    u16 tx_free = Buffer_GetNum(&TxBuffer);
    snprintf(buftext, 38, "Tx                   %u/%u bytes", tx_free, BUFFER_LEN);

    UI_DrawText(1, 11, PAL1, buftext);
    UI_DrawHProgressBar(4, 11, BUFFER_LEN/32, tx_free/32);

    // - Network traffic -
    u32 down = RXBytes;
    u32 rem = 0;
    u32 up = TXBytes;
    char unit = 'B';

    // Down
    if (down >= 1024)
    {
        rem = down % 1024;
        down /= 1024;
        unit = 'K';
    }

    if (down >= 1024)
    {
        rem = down % 1024;
        down /= 1024;
        unit = 'M';
    }

    if (down >= 1024)
    {
        rem = down % 1024;
        down /= 1024;
        unit = 'G';
    }

    if (unit == 'B')
    {
        snprintf(buftext, 20, "Down: %lu bytes", down);
    }
    else snprintf(buftext, 20, "Down: %lu,%03lu %ciB", down, rem > 999 ? 999 : rem, unit);
    UI_DrawText(1, 16, PAL1, buftext);

    // Up
    rem = 0;
    unit = 'B';

    if (up >= 1024)
    {
        rem = up % 1024;
        up /= 1024;
        unit = 'K';
    }

    if (up >= 1024)
    {
        rem = up % 1024;
        up /= 1024;
        unit = 'M';
    }

    if (up >= 1024)
    {
        rem = up % 1024;
        up /= 1024;
        unit = 'G';
    }

    if (unit == 'B')
    {
        snprintf(buftext, 20, "  Up: %lu bytes", up);
    }
    else snprintf(buftext, 20, "  Up: %lu,%03lu %ciB", up, rem > 999 ? 999 : rem, unit);
    UI_DrawText(1, 17, PAL1, buftext);

    
    // - Network speed -
    u32 downspeed = (RXBytes - Lastdown) * 8;
    u32 upspeed = (TXBytes - Lastup) * 8;

    if (downspeed >= 1000)
    {
        snprintf(buftext, 18, "Down: %lu,%03lu Kbps", downspeed/1000, downspeed % 1000);
        UI_DrawText(21, 16, PAL1, buftext);
    }
    else
    {
        snprintf(buftext, 18, "Down: %lu bps", downspeed);
        UI_DrawText(21, 16, PAL1, buftext);
    }
    
    if (upspeed >= 1000)
    {
        snprintf(buftext, 18, "  Up: %lu,%03lu Kbps", upspeed/1000, upspeed % 1000);
        UI_DrawText(21, 16, PAL1, buftext);
    }
    else
    {
        snprintf(buftext, 18, "  Up: %lu bps", upspeed);
        UI_DrawText(21, 17, PAL1, buftext);
    }
    
    Lastdown = RXBytes;
    Lastup = TXBytes;
}

static void DrawDeviceTab()
{
    u8 num_dev[6] =
    {
        0, 0, 0, 0, 0
    };

    UI_DrawGroupBox(0,  2, 16, 6, "Port 1");
    UI_DrawGroupBox(0,  9, 16, 6, "Port 2");
    UI_DrawGroupBox(0, 16, 16, 6, "Port 3");

    UI_DrawGroupBox(20,  2, 16, 6, "Cartridge");
    UI_DrawGroupBox(20,  9, 16, 6, "Expansion");
    UI_DrawGroupBox(20, 16, 16, 6, "Unknown");

    for (u8 s = 0; s < DEV_MAX; s++)
    {
        SM_Device *d = DevList[s];
        if (d != NULL)
        {
            switch (d->PAssign)
            {
                case DP_Port1:
                    UI_DrawText(1, 4+num_dev[0], PAL1, d->Id.sName);
                    num_dev[0]++;
                break;

                case DP_Port2:
                    UI_DrawText(1, 11+num_dev[1], PAL1, d->Id.sName);
                    num_dev[1]++;
                break;

                case DP_Port3:
                    UI_DrawText(1, 18+num_dev[2], PAL1, d->Id.sName);
                    num_dev[2]++;
                break;

                case DP_CART:
                    UI_DrawText(21, 4+num_dev[3], PAL1, d->Id.sName);
                    num_dev[3]++;
                break;

                case DP_EXP:
                    UI_DrawText(21, 11+num_dev[4], PAL1, d->Id.sName);
                    num_dev[4]++;
                break;
            
                case DP_None:
                default:
                    UI_DrawText(21, 18+num_dev[5], PAL1, d->Id.sName);
                    num_dev[5]++;
                break;
            }
        }
    }
}

static void DrawInfoTab()
{
    u8 vreg = *((vu8*) 0xA10001);
    char buf[40];
    u8 i = 2;

    snprintf(buf, 40, "MD Version: %s %s v%u", vreg & 0x40 ? "PAL" : "NTSC", vreg & 0x80 ? (vreg & 0x40 ? "Europe" : "US") : "Japan", vreg & 0xF);
    UI_DrawText(0,  i++, PAL1, buf);

    i++;

    snprintf(buf, 40, "SMDT Version: %s", STATUS_VER_STR);
    UI_DrawText(0,  i++, PAL1, buf);

    snprintf(buf, 40, "  Build date: %s %s", __DATE__, __TIME__);
    UI_DrawText(0,  i++, PAL1, buf);

    i++;

    SecondsToDateTime(&SystemTime, GetTimeSync());
    snprintf(buf, 40, "  Time: %u-%u-%u %02u:%02u:%02u", SystemTime.year, SystemTime.month, SystemTime.day, SystemTime.hour, SystemTime.minute, SystemTime.second);
    UI_DrawText(0,  i++, PAL1, buf);

    SecondsToDateTime(&uptime, SystemUptime);
    snprintf(buf, 40, "Uptime: %u Months, %u Days %02u:%02u:%02u", uptime.month-1, uptime.day-1, uptime.hour, uptime.minute, uptime.second);
    UI_DrawText(0,  i++, PAL1, buf);

    i++;

    snprintf(buf, 40, "Expansion: %s", vreg & 0x20 ? "None               " : "Connected (Type: ?)");
    UI_DrawText(0, i++, PAL1, buf);

    snprintf(buf, 40, "VRAM size: %u KiB", bVRAM_128KB ? 128 : 64);
    UI_DrawText(0, i++, PAL1, buf);
}

static void UpdateView()
{
    UI_Begin(InfoWindow);

    UI_DrawTabs(0, 0, 38, 3, tIdx, sIdx, tab_text);
    UI_ClearRect(0, 2, 38, 21);

    switch (tIdx)
    {
        case 0: DrawSysMonTab(); break;
        case 1: DrawDeviceTab(); break;
        case 2: DrawInfoTab(); break;    
        default: break;
    }

    UI_End();
}

void InfoView_Input()
{
    if (bMouse)
    {
        u16 r = Mouse_GetRect(mrect_data) & 0x7F;

        // Click
        if ((r < 3) && (tIdx != r))
        {
            if (is_KeyUp(sv_MBind_Click))
            {
                tIdx = r;
                UpdateView();
                SecondElapsed(&UpdateCnt, 1);   // Reset UpdateCnt
            }
        }

        // Hover
        if ((sIdx != r) && (is_KeyUp(sv_MBind_Click) == FALSE))
        {
            sIdx = r;
            UI_Begin(InfoWindow);
            UI_DrawTabs(0, 0, 38, 3, tIdx, sIdx, tab_text);
            UI_RepaintRow(3, 1);
            UI_EndNoPaint();
        }
    }

    if (is_KeyUp(KEY_LEFT))
    {
        if (tIdx == 0) tIdx = 2; else tIdx--;

        sIdx = tIdx;
        UpdateView();
    }

    if (is_KeyUp(KEY_RIGHT))
    {
        if (tIdx == 2) tIdx = 0; else tIdx++;
        
        sIdx = tIdx;
        UpdateView();
    }

    // Quick switch tab
    if (is_KeyUp(KEY_TAB))
    {
        if (tIdx < 2) tIdx++; else tIdx = 0;

        sIdx = tIdx;
        UpdateView();
    }

    // Back/Close
    if (is_KeyUp(KEY_ESCAPE) || is_KeyUp(sv_MBind_AltClick))
    {
        WinMgr_Close(W_InfoView);
    }

    // Abuse the input loop for UI draw updates...
    if (SecondElapsed(&UpdateCnt, 1))
    {
        if (tIdx != 1) UpdateView();
    }

    return;
}

static void DrawWindow()
{
    TRM_SetWinHeight(30);
    TRM_ClearArea(0, 1, 40, 26, PAL1, TRM_CLEAR_BG);  // h=27

    UI_Begin(InfoWindow);
    UI_FillRect(0, 27, 40, 2, 0xDE);
    UI_End();

    ScrollY = 0;

    UpdateView();
}

u16 InfoView_Open()
{
    InfoWindow = malloc(sizeof(SM_Window));

    if (InfoWindow == NULL)
    {
        printf("[91mFailed to allocate memory;\nCan't create InfoWindow[0m\n");
        return 1;
    }

    sIdx = -1;
    tIdx = 0;
    Lastdown = RXBytes;
    Lastup = TXBytes;

    UI_CreateWindow(InfoWindow, "System Info - WIP", WF_None);
    DrawWindow();

    return 0;
}

void InfoView_Close()
{
    TRM_SetWinHeight(1);

    // Clear favorite viewer window tiles
    TRM_ClearArea(0, 1, 40, 26 + (bPALSystem?2:0), PAL1, TRM_CLEAR_BG);
    
    // Erase bottom most row tiles (May obscure IRC text input box). 
    // Normally the entire window should be erased by this call, but not all other windows may fill in the erased (black opaque) tiles.
    TRM_ClearArea(0, 27 + (bPALSystem?2:0), 40, 1, PAL1, TRM_CLEAR_INVISIBLE);

    if (InfoWindow != NULL)
    {
        free(InfoWindow);
        InfoWindow = NULL;
    }
    MEM_pack();
}
