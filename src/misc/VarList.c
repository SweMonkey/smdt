#include "VarList.h"
#include "QMenu.h"              // sv_QBGCL & sv_QFGCL
#include "IRC.h"                // sv_Username & sv_QuitStr
#include "Screensaver.h"        // sv_bScreensaver
#include "Terminal.h"
#include "Keyboard.h"           // sv_KeyLayout
#include "devices/XP_Network.h" // sv_ConnTimeout
#include "Cursor.h"             // sv_CursorCL

SM_VarList VarList[] =
{
    {ST_BYTE, &sv_HSOffset,     "hsoffset"},
    {ST_BYTE, &sv_TermType,     "termtype"},
    {ST_SARR, &sv_Baud,         "baud"},
    {ST_BYTE, &sv_TermColumns,  "termcol"},
    {ST_BYTE, &sv_QBGCL,        "qselbg"},
    {ST_BYTE, &sv_QFGCL,        "qselfg"},
    {ST_WORD, &sv_CBGCL,        "custbg"},
    {ST_WORD, &sv_CFG0CL,       "custfg0"},
    {ST_WORD, &sv_CFG1CL,       "custfg1"},
    {ST_BYTE, &sv_Font,         "font"},
    {ST_LONG, &sv_ListenPort,   "listenport"},
    {ST_BYTE, &sv_bWrapAround,  "bwraparound"},
    {ST_BYTE, &sv_KeyLayout,    "kblayout"},
    {ST_SARR, &sv_QuitStr,      "quitstr"},
    {ST_SARR, &sv_Username,     "username"},
    {ST_BYTE, &sv_bHighCL,      "bhighcl"},
    {ST_BYTE, &sv_bScreensaver, "bscreensaver"},
    {ST_LONG, &sv_ConnTimeout,  "conntime"},
    {ST_WORD, &sv_CursorCL,     "cursorcl"},
    {ST_BYTE, &sv_QCURCL,       "qselcrcl"},
    {0, 0, 0}  // Save terminator
};
