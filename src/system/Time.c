#include "Time.h"
#include "Utils.h"

// https://www.epochconverter.com/

SM_Time SystemTime;
s32 SystemUptime = 0;
s32 TimeSync = 0;

static u32 Second = 0;  // 1/60s ticker

bool bTempTime = TRUE;


SM_Time SecondsToDateTime(s32 seconds)
{
    // Calculate HH:MM:SS assuming 31 days a month (Time will be wrong otherwise)
    u32 year   = seconds;
    u32 month  = year   % 32140800ULL;
    u32 day    = month  % 2678400UL;
    u32 hour   = day    % 86400UL;
    u16 minute = hour   % 3600UL;
    u16 second = minute % 60U;

    SM_Time t;
    t.hour   = hour   / 3600UL;
    t.minute = minute / 60U;
    t.second = second;

    // Recalculate date with correct values (Date will be wrong otherwise). 1 month = approx 30.44 days, 1 year = approx 365.24 days
    month   = year   % 31556926ULL;
    day     = month  % 2629743UL;
    t.year  = year   / 31556926ULL;
    t.month = month  / 2629743UL;
    t.day   = day    / 86400UL;

    t.year  += 1970;
    t.month += 1;
    t.day   += 1;

    return t;
}

void TimeToStr_Full(SM_Time t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%lu-%lu-%lu %02lu:%02u:%02u", t.day, t.month, t.year, t.hour, t.minute, t.second);
}

void TimeToStr_Date(SM_Time t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%lu-%lu-%lu", t.day, t.month, t.year);
}

void TimeToStr_Time(SM_Time t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%02lu:%02u:%02u", t.hour, t.minute, t.second);
}

void TickClock()
{
    if ((Second % (bPALSystem?50:60)) == 0)
    {
        SystemUptime++;
        TimeSync++;
        Second++;

        if (bTempTime) SystemTime = SecondsToDateTime(SystemUptime);   // TEMP - should be sync'd to time server
        else SystemTime = SecondsToDateTime(TimeSync);
    }
    else Second++;
}

void SetSystemDateTime(s32 seconds)
{
    TimeSync = seconds;
    SystemTime = SecondsToDateTime(TimeSync);
}