#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <genesis.h>

// Title bar prefix
#define STATUS_VER_STR "v0.34.1"
#define STATUS_TEXT "SMDT v34"
#define STATUS_TEXT_SHORT "SMDT"

#define SMDT_VMAJOR_INT 0
#define SMDT_VMINOR_INT 34
#define SMDT_VREV_INT 1

// VRAM memory addresses for various graphics, in tile units (/32)
#define AVR_BGBLOCK 0       // $0000 - $01FF
#define AVR_CURSOR  0x10    // $0200 - $02DF
#define AVR_POINTER 0x17    // $02E0 - $02FF
#define AVR_ICONS   0x18    // $0300 - $03FF
#define AVR_SCRSAV  0x30    // $0400 - $05FF
#define AVR_FONT0   0x100   // $0800 - $47FF
#define AVR_FONT1   0x300   // $4800 - $87FF
#define AVR_UI      0x40    // $8800 - $9FFF

// VRAM memory addresses for VDP tables
extern u16 AVR_HSCROLL;
#define AVR_HSCROLL_START 0xA000  // $A000 - $A3FF
#define AVR_SAT_START     0xAC00  // $AC00 - $AFFF 
extern u16 AVR_SAT;
#define AVR_WINDOW_START  0xB000  // $B000 - $BFFF
extern u16 AVR_WINDOW;
#define AVR_PLANE_A       0xC000  // $C000 - $DFFF
#define AVR_PLANE_B       0xE000  // $E000 - $FFFF
#define AVR_FONT0_POS     (AVR_FONT0*32)


// Check if a character is printable
#define isPrintable(x) (x>0x1F)

// Tile macro for clearing window plane (Use with TRM_ClearArea)
#define TRM_CLEAR_WINDOW (AVR_UI)   // For clearing inside windows or on the status bar
#define TRM_CLEAR_BG (AVR_UI+0xBE)  // For clearing area to black (opaque)
#define TRM_CLEAR_INVISIBLE (0)     // For clearing area to invisible (Tile 0)

// Debugging
//#define EMU_BUILD 1   // Enable to build specialized debug version meant to run on emulators (networking is disabled, faster startup)
//#define ATT_LOGGING   // Log attribute changes
//#define IRC_LOGGING 1 // Log IRC debug messages (1 = Unhandled CMD only, 2 = LOG EVERYTHING)
//#define TRM_LOGGING   // Log terminal debug messages
//#define IAC_LOGGING   // Log IAC data
//#define ESC_LOGGING 4 // Log ESC data (1 = Log prioritised info, 2 = log misc errors and warnings, 3 = log spam, 4 = log everything spam)
//#define OSC_LOGGING
//#define UTF_LOGGING   // Log UTF-8 messages
//#define KB_DEBUG      // Log keyboard debug messages
//#define GOP_LOGGING
//#define DEBUG_STREAM
//#define SHOW_FRAME_USAGE
#define ENABLE_CLOCK

extern bool bPALSystem;
extern bool bHardReset;

typedef u16 U16Callback(void);
typedef bool BoolCallback(void);

void TRM_SetWinHeight(u8 h);
void TRM_SetWinWidth(u8 w);
void TRM_SetWinParam(bool from_bottom, bool from_right, u8 w, u8 h);
void TRM_ResetWinParam();

void TRM_DrawChar(const u8 c, u8 x, u8 y, u8 palette);
void TRM_DrawText(const char *str, u16 x, u16 y, u8 palette);
void TRM_ClearArea(u16 x, u16 y, u16 w, u16 h, u8 palette, u16 tile);
void TRM_ClearPlane(VDPPlane plane);
void TRM_FillPlane(VDPPlane plane, u16 value);

u8 atoi(char *c);
u16 atoi16(char *c);
u32 atoi32(char *c);
void itoa(s32 n, char s[]);
u8 tolower(u8 c);
char *strtok(char *s, char d);

#define tolower_string(s) u16 _ti = 0;while(s[_ti]){s[_ti]=(char)tolower((u8)s[_ti]);_ti++;}

char *strncat(char *to, const char *from, u16 num);
u16 vsnprintf(char *buf, u16 size, const char *fmt, va_list args);
u16 snprintf(char *buffer, u16 size, const char *fmt, ...) __attribute__ ((format (printf, 3, 4)));

void *memmove(void *dest, const void *src, u32 n);
s32 memcmp(const void *s1, const void *s2, u32 n);

u32 syscall(vu32 n, vu32 a, vu32 b, vu32 c, vu32 d, vu32 e, vu32 f);

// Used by LittleFS
#define true 1
#define false 0
typedef u32 size_t;
#define LFS_NO_ASSERT
#define LFS_NO_DEBUG
#define LFS_NO_WARN
#define LFS_NO_ERROR
#define ORDER_BIG_ENDIAN 1
#define BYTE_ORDER ORDER_BIG_ENDIAN

size_t strspn(const char *str1, const char *str2);
size_t strcspn(const char *str1, const char *str2);
const char *strchr(const char *str, int character);
s16 strncmp(const char *str1, const char *str2, u32 n);

void *realloc(void *ptr, u16 old_size, u16 new_size);

#endif // UTILS_H_INCLUDED
