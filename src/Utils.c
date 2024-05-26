#include "Utils.h"

bool bPALSystem;
static u8 WinBottom = 0, WinRight = 1;
static u8 WinWidth = 0, WinHeight = 1;
static char StatusText[36];


void TRM_SetStatusText(const char *t)
{
    strncpy(StatusText, t, 36);
    TRM_ClearTextArea(0, 0, 36, 1);
    TRM_DrawText(StatusText, 1, 0, PAL1);
}

void TRM_ResetStatusText()
{
    TRM_ClearTextArea(0, 0, 36, 1);
    TRM_DrawText(StatusText, 1, 0, PAL1);
}

void TRM_SetWinHeight(u8 h)
{
    *((vu16*) VDP_CTRL_PORT) = 0x9200 | ((h & 0x7F) | (WinBottom?0x80:0));
}

void TRM_SetWinWidth(u8 w)
{
    *((vu16*) VDP_CTRL_PORT) = 0x9100 | (((w) & 0x7F) | (WinRight?0x80:0));
}

void TRM_SetWinParam(bool from_bottom, bool from_right, u8 w, u8 h)
{
    WinBottom = from_bottom;
    WinRight = from_right;
    WinWidth = w;
    WinHeight = h;
    
    *((vu16*) VDP_CTRL_PORT) = 0x9100 | ((WinWidth  & 0x7F) | (WinRight ?0x80:0));  // Set window width and left/right
    *((vu16*) VDP_CTRL_PORT) = 0x9200 | ((WinHeight & 0x7F) | (WinBottom?0x80:0));  // Set window height and top/bottom
}

void TRM_ResetWinParam()
{
    *((vu16*) VDP_CTRL_PORT) = 0x9100 | ((WinWidth  & 0x7F) | (WinRight ?0x80:0));  // Set window width and left/right
    *((vu16*) VDP_CTRL_PORT) = 0x9200 | ((WinHeight & 0x7F) | (WinBottom?0x80:0));  // Set window height and top/bottom
}

inline void TRM_SetStatusIcon(const char icon, u16 pos)
{
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(VDP_WINDOW + ((pos & 63) * 2));
    *((vu16*) VDP_DATA_PORT) = icon;
}

void TRM_DrawChar(const u8 c, u8 x, u8 y, u8 palette)
{
    *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(VDP_WINDOW + (((x & 63) + ((y & 31) << 6)) * 2));
    *((vu16*) VDP_DATA_PORT) = TILE_ATTR_FULL(palette, 1, 0, 0, AVR_UI + c);
}

void TRM_DrawText(const char *str, u16 x, u16 y, u8 palette)
{
    u16 data[128];
    const u8 *s;
    u16 *d;
    u16 i, len;

    if ((x >= 64) || (y >= 32)) return;

    len = strlen(str);

    // Adjust string length
    if (len > (64 - x)) len = 64 - x;

    // Prepare the data
    s = (const u8*) str;
    d = data;
    i = len;
    while (i--) *d++ = AVR_UI + (*s++);

    VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(palette, 1, 0, 0), y, x, len, DMA);
}

void TRM_ClearTextArea(u16 x, u16 y, u16 w, u16 h)
{
    u16 data[128];
    u16 i, ya;
    u16 wa, ha;

    if ((x >= 64) || (y >= 32)) return;

    // Adjust rectangle width
    wa = w;
    if (wa > (64 - x)) wa = 64 - x;

    // Adjust rectangle height
    ha = h;
    if (ha > (32 - y)) ha = 32 - y;

    // Prepare the data
    memsetU16(data, AVR_UI, wa);

    ya = y;
    i = ha;
    while (i--) VDP_setTileMapDataRowEx(WINDOW, data, TILE_ATTR(PAL1, 1, 0, 0), ya++, x, wa, DMA);
}

void TRM_FillPlane(VDPPlane plane, u16 tile)
{
    switch(plane)
    {
        case BG_A:
            DMA_doVRamFill(VDP_BG_A, 0x2000, 0, 1);
        break;

        case BG_B:
            DMA_doVRamFill(VDP_BG_B, 0x2000, 0, 1);
        break;

        default:
        return;
    }

    VDP_waitDMACompletion();
}

inline u8 atoi(char *c)
{
    u8 value = 0;

    while (*c != '\0')
    {
        value *= 10;
        value += (u8) (*c - '0');
        c++;
    }

    return value;
}

inline u16 atoi16(char *c)
{
    u16 value = 0;

    while (*c != '\0')
    {
        value *= 10;
        value += (u8) (*c - '0');
        c++;
    }

    return value;
}

inline u32 atoi32(char *c)
{
    u32 value = 0;

    while (*c != '\0')
    {
        value *= 10;
        value += (u8) (*c - '0');
        c++;
    }

    return value;
}

inline u8 tolower(u8 c)
{
    return ((c >= 'A') && (c <= 'Z') ? c | 0x60 : c);
}

char *strtok(char *s, char d)
{
    // Stores the state of string
    static char *input = NULL;

    // Initialize the input string
    if (s != NULL) input = s;

    // Case for final token
    if (input == NULL) return NULL;

    // Stores the extracted string
    char *result = malloc(strlen(input) + 1);
    int i = 0;

    // Start extracting string and store it in array
    for (; input[i] != '\0'; i++)
    {
        // If delimiter is not reached then add the current character to result[i]
        if (input[i] != d) result[i] = input[i];
        else // Else store the string formed
        {
            result[i] = '\0';
            input = input + i + 1;
            return result;
        }
    }

    // Case when loop ends
    result[i] = '\0';
    input = NULL;

    // Return the resultant pointer to the string
    return result;
}

#define SYSCALL_RET(x) {n=x;return 0;}
u32 syscall(register vu32 n, register vu32 a, register vu32 b, register vu32 c, register vu32 d, register vu32 e, register vu32 f)
{
    kprintf("syscall\nn = $%lX\na = $%lX\nb = $%lX\nc = $%lX\nd = $%lX\ne = $%lX\nf = $%lX", n, a, b, c, d, e, f);

    return 0;
}

// strncat, modified strcat from SGDK
char *strncat(char *to, const char *from, u16 num)
{
    const char *src;
    char *dst;
    u16 i = strlen(to) + strlen(from);
    i = (i > num ? num : i);

    src = from;
    dst = to;
    while (*dst++){i--;}

    --dst;
    while ((i--) && (*dst++ = *src++));

    return to;
}

// snprintf, modified sprintf from SGDK
static const char const uppercase_hexchars[] = "0123456789ABCDEF";
static const char const lowercase_hexchars[] = "0123456789abcdef";

static u16 skip_atoi(const char **s)
{
    u16 i = 0;

    while(isdigit(**s)) i = (i * 10) + *((*s)++) - '0';

    return i;
}

u16 vsnprintf(char *buf, u16 size, const char *fmt, va_list args)
{
    char tmp_buffer[14];
    s32 i;
    s16 len;
    s32 *ip;
    u32 num;
    char *s;
    const char *hexchars;
    char *str;
    s16 left_align;
    s16 plus_sign;
    s16 zero_pad;
    s16 space_sign;
    s16 field_width;
    s16 precision;
    
    s32 left = size;

    for (str = buf; *fmt; ++fmt)
    {
        if (left <= 0) break;

        if (*fmt != '%')
        {
            left--;
            *str++ = *fmt;
            continue;
        }

        space_sign = zero_pad = plus_sign = left_align = 0;

        // Process the flag
repeat:
        ++fmt;          // this also skips first '%'

        switch (*fmt)
        {
        case '-':
            left_align = 1;
            goto repeat;

        case '+':
            plus_sign = 1;
            goto repeat;

        case ' ':
            if ( !plus_sign )
                space_sign = 1;

            goto repeat;

        case '0':
            zero_pad = 1;
            goto repeat;
        }

        left--;

        // Process field width and precision

        field_width = precision = -1;

        if (isdigit(*fmt))
            field_width = skip_atoi(&fmt);
        else if (*fmt == '*')
        {
            ++fmt;
            left--;
            // it's the next argument
            field_width = va_arg(args, s32);

            if (field_width < 0)
            {
                field_width = -field_width;
                left_align = 1;
            }
        }

        if (*fmt == '.')
        {
            ++fmt;
            left--;

            if (isdigit(*fmt))
                precision = skip_atoi(&fmt);
            else if (*fmt == '*')
            {
                ++fmt;
                left--;
                // it's the next argument
                precision = va_arg(args, s32);
            }

            if (precision < 0)
                precision = 0;
        }

        if ((*fmt == 'h') || (*fmt == 'l') || (*fmt == 'L'))
        {
            ++fmt;
            //left--; // hmmm
        }

        if (left_align)
            zero_pad = 0;

        switch (*fmt)
        {
        case 'c':
            if (!left_align)
                while(--field_width > 0)
                {
                    *str++ = ' ';
                    left--;
                    if (left <= 0) break;
                }

            *str++ = (unsigned char) va_arg(args, s32);
            //left--;

            while(--field_width > 0)
            {
                *str++ = ' ';
                left--;
                if (left <= 0) break;
            }

            continue;

        case 's':
            s = va_arg(args, char *);

            if (!s)
                s = "<NULL>";

            len = strnlen(s, precision);

            if (!left_align)
                while(len < field_width--)
                {
                    *str++ = ' ';
                    left--;
                    if (left <= 0) break;
                }

            for (i = 0; i < len; ++i)
            {
                *str++ = *s++;
                left--;
                if (left <= 0) break;
            }

            while(len < field_width--)
            {
                *str++ = ' ';
                left--;
                if (left <= 0) break;
            }

            continue;

        case 'p':
            if (field_width == -1)
            {
                field_width = 2 * sizeof(void *);
                zero_pad = 1;
            }

            hexchars = uppercase_hexchars;
            goto hexa_conv;

        case 'x':
            hexchars = lowercase_hexchars;
            goto hexa_conv;

        case 'X':
            hexchars = uppercase_hexchars;

hexa_conv:
            s = &tmp_buffer[12];
            *--s = 0;
            num = va_arg(args, u32);

            if (!num)
                *--s = '0';

            while(num)
            {
                *--s = hexchars[num & 0xF];
                num >>= 4;
            }

            num = plus_sign = 0;

            break;

        case 'n':
            ip = va_arg(args, s32*);
            *ip = (str - buf);
            continue;

        case 'u':
            s = &tmp_buffer[12];
            *--s = 0;
            num = va_arg(args, u32);

            if (!num)
                *--s = '0';

            while(num)
            {
                *--s = (num % 10) + 0x30;
                num /= 10;
            }

            num = plus_sign = 0;

            break;

        case 'd':
        case 'i':
            s = &tmp_buffer[12];
            *--s = 0;
            i = va_arg(args, s32);

            if (!i)
                *--s = '0';

            if (i < 0)
            {
                num = 1;

                while(i)
                {
                    *--s = 0x30 - (i % 10);
                    i /= 10;
                }
            }
            else
            {
                num = 0;

                while(i)
                {
                    *--s = (i % 10) + 0x30;
                    i /= 10;
                }
            }

            break;

        default:
            continue;
        }

        len = strnlen(s, precision);

        if (num)
        {
            *str++ = '-';
            field_width--;
            left--;
        }
        else if (plus_sign)
        {
            *str++ = '+';
            field_width--;
            left--;
        }
        else if (space_sign)
        {
            *str++ = ' ';
            field_width--;
            left--;
        }
        
        ///if (left <= 0) break;

        if ( !left_align)
        {
            if (zero_pad)
            {
                while(len < field_width--)
                {
                    *str++ = '0';
                    //left--;
                    //if (left <= 0) break;
                }
            }
            else
            {
                while(len < field_width--)
                {
                    if (left <= 0) break;
                    *str++ = ' ';
                    left--;
                }
            }
        }

        for (i = 0; i < len; ++i)
        {
            //if (left <= 0) break;
            *str++ = *s++;
            left--;
        }

        while(len < field_width--)
        {
            if (left <= 0) break;
            *str++ = ' ';
            left--;
        }
    }

    *str = '\0';

    return str - buf;
}

u16 snprintf(char *buffer, u16 size, const char *fmt, ...)
{
    va_list args;
    u16 i;

    va_start(args, fmt);
    i = vsnprintf(buffer, size, fmt, args);
    va_end(args);

    return i;
}
