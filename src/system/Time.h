#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED

#include <genesis.h>

typedef struct
{
    u16 year;
    u8 month;
    u8 day;
    u8 hour;
    u8 minute;
    u8 second;
} SM_Time;

extern SM_Time SystemTime;
extern u32 SystemUptime;
extern s8 sv_TimeZone;
extern char sv_TimeServer[];
extern u16 sv_EpochStart;

void SecondsToDateTime(SM_Time *t, u32 seconds);
void TimeToStr_Full(SM_Time *t, char *ret);
void TimeToStr_Date(SM_Time *t, char *ret);
void TimeToStr_Time(SM_Time *t, char *ret);
void TimeToStr_TimeNoSec(SM_Time *t, char *ret);

void TickClock();
void SetSystemDateTime(u32 seconds);
u32 GetTimeSinceLastSync();
u32 GetTimeSync();
u8 DoTimeSync(char *server);

#endif // TIME_H_INCLUDED
