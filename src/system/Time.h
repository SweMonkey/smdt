#ifndef TIME_H_INCLUDED
#define TIME_H_INCLUDED

#include <genesis.h>

typedef struct s_time
{
    u32 year;
    u32 month;
    u32 day;
    u32 hour;
    u16 minute;
    u16 second;
} SM_Time;

extern SM_Time SystemTime;
extern s32 SystemUptime;
extern s8 sv_TimeZone;
extern char sv_TimeServer[];
extern u16 sv_EpochStart;

SM_Time SecondsToDateTime(s32 seconds);
void TimeToStr_Full(SM_Time t, char *ret);
void TimeToStr_Date(SM_Time t, char *ret);
void TimeToStr_Time(SM_Time t, char *ret);

void TickClock();
void SetSystemDateTime(s32 seconds);
s32 GetTimeSinceLastSync();
u8 DoTimeSync(char *server);

#endif // TIME_H_INCLUDED
