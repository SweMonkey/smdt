#ifndef DLOG_H_INCLUDED
#define DLOG_H_INCLUDED

#include <genesis.h>

extern u32 RXBytes;

// Logging levels
#define LOG_ERROR   1
#define LOG_WARNING 2
#define LOG_INFO    4
#define LOG_DEBUG   8

#define LOG_NONE    0
#define LOG_NORMAL  (LOG_ERROR | LOG_WARNING)
#define LOG_VERBOSE (LOG_ERROR | LOG_WARNING | LOG_INFO)
#define LOG_ALL     (LOG_ERROR | LOG_WARNING | LOG_INFO | LOG_DEBUG)

// Logging configuration
#define LOG_ESC LOG_NONE    // Escape parser    - Options: ALL
#define LOG_IRC LOG_NONE    // IRC parser       - Options: ALL
#define LOG_IAC LOG_NONE    // IAC parser       - Options: ERROR or INFO
#define LOG_ATT LOG_NONE    // Attribute parser - Options: INFO
#define LOG_OSC LOG_NONE    // OS Control       - Options: ERROR or INFO
#define LOG_UTF LOG_NONE    // UTF-8 Debug      - Options: INFO

// Common log printing
#define LOG_PRINT(color, tag, fmt, ...) kprintf(color tag "[0x%lX] " fmt "\e[0m", (u32)(RXBytes - 1), ##__VA_ARGS__)

// ESC
#if (LOG_ESC & LOG_ERROR)
#define ESC_ERROR(fmt, ...) LOG_PRINT("\e[91m", "[ESC][ERROR]", fmt, ##__VA_ARGS__)
#else
#define ESC_ERROR(fmt, ...) ((void)0)
#endif

#if (LOG_ESC & LOG_WARNING)
#define ESC_WARN(fmt, ...) LOG_PRINT("\e[93m", "[ESC][WARN ]", fmt, ##__VA_ARGS__)
#else
#define ESC_WARN(fmt, ...) ((void)0)
#endif

#if (LOG_ESC & LOG_INFO)
#define ESC_INFO(fmt, ...) LOG_PRINT("\e[37m", "[ESC][INFO ]", fmt, ##__VA_ARGS__)
#else
#define ESC_INFO(fmt, ...) ((void)0)
#endif

#if (LOG_ESC & LOG_DEBUG)
#define ESC_DEBUG(fmt, ...) LOG_PRINT("\e[92m", "[ESC][DEBUG]", fmt, ##__VA_ARGS__)
#else
#define ESC_DEBUG(fmt, ...) ((void)0)
#endif

// IRC
#if (LOG_IRC & LOG_ERROR)
#define IRC_ERROR(fmt, ...) LOG_PRINT("\e[91m", "[IRC][ERROR]", fmt, ##__VA_ARGS__)
#else
#define IRC_ERROR(fmt, ...) ((void)0)
#endif

#if (LOG_IRC & LOG_WARNING)
#define IRC_WARN(fmt, ...) LOG_PRINT("\e[93m", "[IRC][WARN ]", fmt, ##__VA_ARGS__)
#else
#define IRC_WARN(fmt, ...) ((void)0)
#endif

#if (LOG_IRC & LOG_INFO)
#define IRC_INFO(fmt, ...) LOG_PRINT("\e[37m", "[IRC][INFO ]", fmt, ##__VA_ARGS__)
#else
#define IRC_INFO(fmt, ...) ((void)0)
#endif

#if (LOG_IRC & LOG_DEBUG)
#define IRC_DEBUG(fmt, ...) LOG_PRINT("\e[92m", "[IRC][DEBUG]", fmt, ##__VA_ARGS__)
#else
#define IRC_OK(fmt, ...) ((void)0)
#endif

// IAC
#if (LOG_IAC & LOG_INFO)
#define IAC_INFO(fmt, ...) LOG_PRINT("\e[95m", "[IAC][INFO ]", fmt, ##__VA_ARGS__)
#else
#define IAC_INFO(fmt, ...) ((void)0)
#endif

#if (LOG_IAC & LOG_ERROR)
#define IAC_ERROR(fmt, ...) LOG_PRINT("\e[91m", "[IAC][ERROR]", fmt, ##__VA_ARGS__)
#else
#define IAC_ERROR(fmt, ...) ((void)0)
#endif

// ATT
#if (LOG_ATT & LOG_INFO)
#define ATT_LOG(fmt, ...) LOG_PRINT("\e[37m", "[ATT]", fmt, ##__VA_ARGS__)
#else
#define ATT_LOG(fmt, ...) ((void)0)
#endif

// OSC
#if (LOG_OSC & LOG_INFO)
#define OSC_INFO(fmt, ...) LOG_PRINT("\e[37m", "[OSC][INFO ]", fmt, ##__VA_ARGS__)
#else
#define OSC_INFO(fmt, ...) ((void)0)
#endif

#if (LOG_OSC & LOG_ERROR)
#define OSC_ERROR(fmt, ...) LOG_PRINT("\e[91m", "[OSC][ERROR]", fmt, ##__VA_ARGS__)
#else
#define OSC_ERROR(fmt, ...) ((void)0)
#endif

// UTF
#if (LOG_UTF & LOG_INFO)
#define UTF_LOG(fmt, ...) LOG_PRINT("\e[37m", "[UTF]", fmt, ##__VA_ARGS__)
#else
#define UTF_LOG(fmt, ...) ((void)0)
#endif

#endif // DLOG_H_INCLUDED