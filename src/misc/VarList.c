#include "VarList.h"
#include "QMenu.h"              // sv_QBGCL & sv_QFGCL
#include "IRC.h"                // sv_Username, sv_QuitStr, sv_IRCFont
#include "Screensaver.h"        // sv_bScreensaver
#include "Terminal.h"           // 
#include "Telnet.h"             // sv_AllowRemoteEnv
#include "Keyboard.h"           // sv_KeyLayout
#include "devices/XP_Network.h" // sv_ConnTimeout, sv_ReadTimeout, sv_DelayTime
#include "devices/RL_Network.h" // sv_DLM, sv_DLL
#include "Cursor.h"             // sv_CursorCL

extern u8 sv_IRCFont;
extern u8 sv_TelnetFont;
extern u8 sv_TerminalFont;
extern u8 sv_ThemeUI;

SM_VarList VarList[] =
{
    {ST_BYTE, &sv_HSOffset,       "hsoffset"},
    {ST_BYTE, &sv_TermType,       "termtype"},
    {ST_SARR, &sv_Baud,           "baud"},
    {ST_BYTE, &sv_TermColumns,    "termcol"},
    {ST_BYTE, &sv_QBGCL,          "qselbg"},
    {ST_BYTE, &sv_QFGCL,          "qselfg"},
    {ST_WORD, &sv_CBGCL,          "custbg"},
    {ST_WORD, &sv_CFG0CL,         "custfg0"},
    {ST_WORD, &sv_CFG1CL,         "custfg1"},
    {ST_LONG, &sv_ListenPort,     "listenport"},
    {ST_BYTE, &sv_KeyLayout,      "kblayout"},
    {ST_SARR, &sv_QuitStr,        "quitstr"},
    {ST_SARR, &sv_Username,       "username"},
    {ST_BYTE, &sv_bHighCL,        "bhighcl"},
    {ST_BYTE, &sv_bScreensaver,   "bscreensaver"},
    {ST_LONG, &sv_ConnTimeout,    "ctime"},
    {ST_WORD, &sv_CursorCL,       "cursorcl"},
    {ST_BYTE, &sv_QCURCL,         "qselcrcl"},
    {ST_BYTE, &sv_BoldFont,       "boldfont"},
    {ST_BYTE, &sv_AllowRemoteEnv, "remoteenv"},
    {ST_LONG, &sv_ReadTimeout,    "rtime"},
    {ST_LONG, &sv_DelayTime,      "dtime"},
    {ST_BYTE, &sv_DLM,            "dlm"},
    {ST_BYTE, &sv_DLL,            "dll"},
    {ST_BYTE, &sv_IRCFont,        "ircfont"},
    {ST_BYTE, &sv_TelnetFont,     "telnetfont"},
    {ST_BYTE, &sv_TerminalFont,   "termfont"},
    {ST_BYTE, &sv_ThemeUI,        "themeui"},
    {0, 0, 0}  // List terminator
};
