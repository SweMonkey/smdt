#include "UTF8.h"
#include "Telnet.h" // NextByte / NC_Data
#include "Utils.h"  // UTF_LOGGING

// https://www.utf8-chartable.de/unicode-utf8-table.pl
// https://www.ascii-code.com/CP437

void TTY_PrintChar(u8 c);

u8 UTF_Bytes;
static u8 UTF_Buffer[4] = {0,0,0,0};
static u8 UTF_Seq = 0;
extern u32 RXBytes;


// Dummy/empty (Pos: 0x00)
static const u8 TB_00[] =
{
    //0   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 80-8F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 90-9F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // A0-AF
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // B0-BF
};

// 0xC0
/*static const u8 TB_C0[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, // A0-AF
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F, // B0-BF
};*/

// 0xC1
static const u8 TB_C1[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, // A0-AF
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F, // B0-BF
};

// 0xC2
static const u8 TB_C2[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0xAD, 0x9B, 0x9C, 0x3F, 0x9D, 0x7C, 0x15, 0x3F, 0x3F, 0xA6, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // A0-AF    A6 = BROKEN BAR (Using VERTICAL BAR since it looks the same as BROKEN BAR in smdt)
    0x3F, 0x3F, 0xFD, 0x3F, 0x3F, 0xE6, 0x14, 0xFA, 0x3F, 0x3F, 0xA7, 0x3F, 0xAC, 0xAB, 0x3F, 0x3F, // B0-BF
};

// 0xC3
static const u8 TB_C3[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x3F, 0x3F, 0x3F, 0x3F, 0x8E, 0x3F, 0x92, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x99, 0x3F, 0x3F, 0x97, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F    99 = `SMALL LETTER U WITH GRAVE` (should be `CAPITAL LETTER U WITH GRAVE` but it is not in ASCII)
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x91, 0x3F, 0x3F, 0x3F, 0x3F, 0x89, 0x8D, 0xA1, 0x8C, 0x3F, // A0-AF
    0x3F, 0xA4, 0x95, 0x3F, 0x3F, 0x3F, 0x94, 0xF6, 0x3F, 0x3F, 0x3F, 0x96, 0x3F, 0x3F, 0x3F, 0x3F, // B0-BF
};

// 0xC4
static const u8 TB_C4[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x41, 0x61, 0x41, 0x61, 0x41, 0x61, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F    80-85 = replaced with `A` and `a` (proper char is not in ASCII)
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x45, 0x65, 0x45, 0x65, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F    98-9B = replaced with `E` and `e` (proper char is not in ASCII)
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // A0-AF
    0x49, 0x3F, 0x3F, 0x3F, 0x4A, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x6C, 0x3F, 0x3F, 0x3F, 0x3F, 0x4C, // B0-BF    B0/B4/BC/BF = replaced with normal character (proper char is not in ASCII)
};

// 0xC6
static const u8 TB_C6[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F
    0x3F, 0x3F, 0x9F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F
    0x3F, 0xA2, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // A0-AF    A1 = `LATIN SMALL LETTER O WITH ACUTE` (should be `LATIN SMALL LETTER O WITH HORN` but it is not in ASCII)
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // B0-BF
};

// 0xCE
static const u8 TB_CE[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F
    0x3F, 0x3F, 0x3F, 0xE2, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F
    0x3F, 0x3F, 0x3F, 0xE4, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0xEA, 0x3F, 0x3F, 0x3F, 0xEE, 0x3F, 0x3F, // A0-AF    AD = `GREEK SMALL LETTER EPSILON` (should be `GREEK SMALL LETTER EPSILON WITH TONOS` but it is not in ASCII)
    0x3F, 0xE0, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // B0-BF
};

// 0xCF
static const u8 TB_CF[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xE3, 0x3F, 0x3F, 0xE5, 0xE7, 0x3F, 0xED, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x58, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // A0-AF  AA = `UPPERCASE X` (should be `COPTIC CAPITAL LETTER GANGIA` but it is not in ASCII)
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // B0-BF
};
/*
// 0xC
static const u8 TB_C[] =
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 80-8F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // 90-9F
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // A0-AF
    0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // B0-BF
};
*/

const u8 UTFTB_E0[][64] =
{
// 0x94 (Pos: 0x00)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xC4, 0xC4, 0xB3, 0xB3, 0xC4, 0xC4, 0xB3, 0xB3, 0xC4, 0xC4, 0xB3, 0xB3, 0xDA, 0xDA, 0xDA, 0xDA, // 80-8F
    0xBF, 0xBF, 0xBF, 0xBF, 0xC0, 0xC0, 0xC0, 0xC0, 0xD9, 0xD9, 0xD9, 0xD9, 0xC3, 0xC3, 0xC3, 0xC3, // 90-9F
    0xC3, 0xC3, 0xC3, 0xC3, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xB4, 0xC2, 0x20, 0x20, 0x20, // A0-AF    AC = 0xC2
    0x20, 0x20, 0x20, 0x20, 0xC1, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xC5, 0x20, 0x20, 0x20, // B0-BF
},

// 0x95 (Pos: 0x01)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0xCD, 0xBA, 0xD5, 0xD6, 0xC9, 0xB8, 0xB7, 0xBB, 0xD4, 0xD3, 0xC8, 0xBE, 0xBD, 0xBC, 0xC6, 0xC7, // 90-9F
    0xCC, 0xB5, 0xB6, 0xB9, 0xD1, 0xD2, 0xCB, 0xCF, 0xD0, 0xCA, 0xD8, 0xD7, 0xCE, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x96 (Pos: 0x02) - U+2500
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0xDF, 0x20, 0x20, 0x20, 0xDC, 0x20, 0x20, 0x20, 0xDB, 0x20, 0x20, 0x20, 0xDD, 0x20, 0x20, 0x20, // 80-8F
    0xDE, 0xB0, 0xB1, 0xB2, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0xFE, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x1E, 0x20, 0x1E, 0x20, 0x10, 0x20, 0x10, 0x20, 0x10, 0x20, 0x1F, 0x20, 0x1F, 0x20, // B0-BF
},

// 0x97 (Pos: 0x03) - U+2500
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x11, 0x20, 0x11, 0x20, 0x11, 0x20, 0x20, 0x20, 0x20, 0x20, 0x04, 0x09, 0x20, 0x20, 0x20, 0x07, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x98 (Pos: 0x04)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x01, 0x0F, 0x20, 0x20, 0x20, // B0-BF    - BC = WHITE SUN WITH RAYS
},

// 0x99 (Pos: 0x05)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x0B, 0x20, 0x20, 0x20, 0x20, 0x20, 0xE6, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F    - 82 TEST ($E6 ?)
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x06, 0x20, 0x20, 0x05, 0x20, 0x03, 0x04, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x9A (Pos: 0x06)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x9B (Pos: 0x07)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x9C (Pos: 0x08)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0xFB, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0xBF (Pos: 0x09)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xFE, 0x09, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// Dummy/empty (Pos: 0x0A)
{
    //0   1    2    3    4    5    6    7    8    9    A    B    C    D    E    F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 80-8F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // 90-9F
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // A0-AF
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', // B0-BF
},

// 0x80 (Pos: 0x0B)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x07, 0x20, 0x20, 0x20, 0xFA, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xFE, 0x09, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0xBB (Pos: 0x0C)
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF    - $BF = U+2EFF
},

// 0x88 (Pos: 0x0D) - U+2190
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x5C, 0x2A, 0x09, 0x07, 0xFB, 0x20, 0x20, 0x20, 0xEC, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xEF, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},

// 0x20 (Pos: 0x0E) - U+0000
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xFA, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},
// 0x86 (Pos: 0x0F) - U+2190 Arrows
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x1B, 0x18, 0x1A, 0x19, 0x1D, 0x12, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x17, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF    - $A8 = UP DOWN ARROW WITH BASE
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},
// 0x89 (Pos: 0x10) - U+
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xF7, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0xF0, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},
// 0x81 (Pos: 0x11) - U+
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0xFC, // B0-BF
},
// 0x8C (Pos: 0x12) - U+
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x7F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0xF4, 0xF5, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},
// 0x82 (Pos: 0x13) - U+
{
    // 0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 80-8F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // 90-9F
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x9E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // A0-AF
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, // B0-BF
},
};

u8 const * const UTF_TablePtr_2seq[] =
{
    //  0      1      2      3      4      5      6      7      8      9      A      B      C      D      E      F
    TB_C2, TB_C1, TB_C2, TB_C3, TB_C4, TB_00, TB_C6, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_CE, TB_CF, // 0xC0-0xCF
    TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, TB_00, // 0xD0-0xDF
};

u8 const * const UTF_TablePtr_E0[256] =
{
    //        0            1            2            3            4            5            6            7            8            9            A            B            C            D            E            F
    UTFTB_E0[0x0E], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x00-0x0F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x10-0x1F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x20-0x2F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x30-0x3F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x40-0x4F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x50-0x5F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x60-0x6F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x70-0x7F

    UTFTB_E0[0x0B], UTFTB_E0[0x11], UTFTB_E0[0x13], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0F], UTFTB_E0[0x0A], UTFTB_E0[0x0D], UTFTB_E0[0x10], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x12], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x80-0x8F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x00], UTFTB_E0[0x01], UTFTB_E0[0x02], UTFTB_E0[0x03], UTFTB_E0[0x04], UTFTB_E0[0x05], UTFTB_E0[0x06], UTFTB_E0[0x07], UTFTB_E0[0x08], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0x90-0x9F
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0xA0-0xAF
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0C], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x09], // 0xB0-0xBF
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0xC0-0xCF
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0xD0-0xDF
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0xE0-0xEF
    UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], UTFTB_E0[0x0A], // 0xF0-0xFF
};


void UTF8_Init()
{
    UTF_Buffer[0] = 0;
    UTF_Buffer[1] = 0;
    UTF_Buffer[2] = 0;
    UTF_Buffer[3] = 0;
    UTF_Seq = 0;

    UTF_Bytes = 0;
}

#ifdef UTF_LOGGING
// From Sik, https://gendev.spritesmind.net/forum/viewtopic.php?f=2&t=2480
u32 Decode_UTF8(const u8 *text)
{
    u8 ch = (u8)(*text);
   
   if (ch < 0x80) 
   {
      return *text;
   }

   if (ch < 0xE0) 
   {
      return (text[0] & 0x1F) << 6 |
             (text[1] & 0x3F);
   }
   
   if (ch < 0xF0) 
   {
      return (text[0] & 0x0F) << 12 |
             (text[1] & 0x3F) << 6 |
             (text[2] & 0x3F);
   }
   
   return (text[0] & 0x07) << 18 |
          (text[1] & 0x3F) << 12 |
          (text[2] & 0x3F) << 6 |
          (text[3] & 0x3F);
}
#endif

void DoUTF8(u8 byte)
{
    UTF_Buffer[UTF_Seq++] = byte;

    if (UTF_Seq >= UTF_Bytes)
    {        
        // Check if bytes[1..3] actually start with b10xxxxxx. If not, then the current UTF_Buffer is not an UTF-8 sequence and it should be passed back into the terminal parser again
        for (u8 i = 1; i < UTF_Seq; i++)
        {
            if ((UTF_Buffer[i] & 0xC0) != 0x80)
            {
                #ifdef UTF_LOGGING
                kprintf(" -- Found non-UTF8 byte ---------------------------------------------------------------------------");
                #endif 

                for (u8 b = 0; b < UTF_Seq; b++)
                {
                    NextByte = (NextByte != NC_UTF8 ? NextByte : NC_SkipUTF);   //(NextByte == NC_Escape ? NC_Escape : NC_SkipUTF);
                    TELNET_ParseRX(UTF_Buffer[b]);
    
                    #ifdef UTF_LOGGING
                    kprintf("Skipping non-UTF8 byte: $%X - Byte that caused skipping: $%X - Position. $%lX", UTF_Buffer[b], UTF_Buffer[i], RXBytes);
                    #endif
                }

                UTF_Seq = 0;
                UTF_Bytes = 0;

                return;    
            }
        }

        u8 FirstByte = (UTF_Buffer[0] & 0xF0);
        u8 OutChar = ' ';

        switch (FirstByte)
        {
            case 0xC0:
                OutChar = UTF_TablePtr_2seq[UTF_Buffer[0] & 0x1F][UTF_Buffer[1] & 0x3F];
                
                #ifdef UTF_LOGGING
                if ((OutChar == 0) || (OutChar == 0x3F)) kprintf("Unfilled UTF8 character: $%X $%X (Codepoint: U+%04lX) @ $%lX", (UTF_Buffer[0] & 0x1F) + 0xC0, (UTF_Buffer[1] & 0x3F) + 0x80, Decode_UTF8(UTF_Buffer), RXBytes);
                #endif

                UTF_Buffer[0] = 0;
                UTF_Buffer[1] = 0;
            break;

            case 0xE0:
                OutChar = UTF_TablePtr_E0[UTF_Buffer[1]][(UTF_Buffer[2]-0x80) & 0x3F];
                
                #ifdef UTF_LOGGING
                if ((OutChar == 0) || (OutChar == 0x20)) kprintf("Unfilled UTF8 character: $%X $%X $%X (Codepoint: U+%04lX)", (UTF_Buffer[0] & 0x1F) + 0xE0, UTF_Buffer[1] & 0x3F, UTF_Buffer[2] & 0x3F, Decode_UTF8(UTF_Buffer));
                #endif

                UTF_Buffer[0] = 0;
                UTF_Buffer[1] = 0;
                UTF_Buffer[2] = 0;
            break;

            case 0xF0:
                OutChar = '?';

                #ifdef UTF_LOGGING
                kprintf("Unfilled UTF8 character: $%X $%X $%X $%X (Codepoint: U+%04lX)", (UTF_Buffer[0] & 0x1F) + 0xF0, UTF_Buffer[1] & 0x3F, UTF_Buffer[2] & 0x3F, UTF_Buffer[3] & 0x3F, Decode_UTF8(UTF_Buffer));
                #endif

                UTF_Buffer[0] = 0;
                UTF_Buffer[1] = 0;
                UTF_Buffer[2] = 0;
                UTF_Buffer[3] = 0;
            break;
        
            default:
            break;
        }

        NextByte = NC_Data;
        UTF_Seq = 0;
        UTF_Bytes = 0;

        TTY_PrintChar(OutChar);
    }

    return;
}
