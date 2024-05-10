#include "VarList.h"
#include "QMenu.h"              // QSelected_BGCL & QSelected_FGCL
#include "IRC.h"                // vUsername & vQuitStr
#include "Screensaver.h"        // bScreensaver
#include "Terminal.h"
#include "Keyboard.h"           // vKB_Layout
#include "devices/XP_Network.h" // vConn_time
#include "Cursor.h"             // Cursor_CL

SM_VarList VarList[] =
{
    {ST_BYTE, &D_HSCROLL,       "hscroll"},
    {ST_BYTE, &vTermType,       "vtermtype"},
    {ST_SARR, vSpeed,           "vspeed"},
    {ST_BYTE, &TermColumns,     "termcol"},
    {ST_BYTE, &QSelected_BGCL,  "qselbg"},
    {ST_BYTE, &QSelected_FGCL,  "qselfg"},
    {ST_WORD, &Custom_BGCL,     "custbg"},
    {ST_WORD, &Custom_FG0CL,    "custfg0"},
    {ST_WORD, &Custom_FG1CL,    "custfg1"},
    {ST_BYTE, &FontSize,        "fontsize"},
    {ST_LONG, &DEV_UART_PORT,   "defuart"},
    {ST_BYTE, &bWrapAround,     "bwraparound"},
    {ST_BYTE, &vKB_Layout,      "vkblayout"},
    {ST_SARR, &vQuitStr,        "vquitstr"},
    {ST_SARR, vUsername,        "vusername"},
    {ST_BYTE, &bHighCL,         "bhighcl"},
    {ST_BYTE, &bScreensaver,    "bscreensaver"},
    {ST_LONG, &vConn_time,      "vconntime"},
    {ST_WORD, &Cursor_CL,       "cursorcl"},
    {ST_BYTE, &QSelected_CURCL, "qselcrcl"},
    {0, 0, 0}  // Save terminator
};
