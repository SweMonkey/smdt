#include "Time.h"
#include "Utils.h"
#include "Network.h"

// https://www.epochconverter.com/

SM_Time SystemTime;
u32 SystemUptime = 0;            // How long the system has been running (Seconds)

static u32 TimeSync = 0;      // Synchronized time (Seconds)
static u32 LastTimeSync = 0;  // Time when last synchronized (Seconds)
static u32 FrameTick = 0;     // 1/50(PAL) or 1/60(NTSC) ticker

s8 sv_TimeZone = 2;           // Time zone - UTC -12 +14
char sv_TimeServer[32] = "time.nist.gov:37"; // Time sync server
u16 sv_EpochStart = 1900;

static const u8 days_in_month[12] = 
{
    31, 28, 31, 30, 31, 30,
    31, 31, 30, 31, 30, 31
};


static inline u32 is_leap_year(u16 year) 
{
    return ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0);
}

void SecondsToDateTime(SM_Time *t, u32 seconds)
{
    u32 secs = seconds;

    t->second = secs % 60; secs /= 60;
    t->minute = secs % 60; secs /= 60;
    t->hour   = secs % 24; secs /= 24;

    // secs = number of days since sv_EpochStart (1900-01-01 or 1970-01-01)
    u32 days = secs;
    u16 year = sv_EpochStart;

    // Count full years
    for (;; year++) 
    {
        u32 days_in_year = is_leap_year(year) ? 366 : 365;
        if (days < days_in_year) break;
        days -= days_in_year;
    }

    t->year = year;

    // Find month and day
    const u8 *dim = days_in_month;
    u8 month = 0;

    for (month = 0; month < 12; month++) 
    {
        u8 dim_this = dim[month];
        if (month == 1 && is_leap_year(year)) // Adjust Feb for leap year
            dim_this++;
    
        if (days < dim_this) break;
    
        days -= dim_this;
    }

    t->month = month + 1;
    t->day   = days + 1;
}

void SecondsTo_HHMM(SM_Time *t, u32 seconds)
{
    u32 secs = seconds;

    t->minute = (secs / 60) % 60;
    t->hour   = (secs / 3600) % 24;
}

void SecondsTo_MM(SM_Time *t, u32 seconds)
{
    u32 secs = seconds;

    t->minute = (secs / 60) % 60;
}

void TimeToStr_Full(SM_Time *t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%u-%u-%u %02u:%02u:%02u", t->day, t->month, t->year, t->hour, t->minute, t->second);
}

void TimeToStr_Date(SM_Time *t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%u-%u-%u", t->day, t->month, t->year);
}

void TimeToStr_Time(SM_Time *t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%02u:%02u:%02u", t->hour, t->minute, t->second);
}

void TimeToStr_TimeNoSec(SM_Time *t, char *ret)
{
    if (ret == NULL) return;

    sprintf(ret, "%02u:%02u", t->hour, t->minute);
}

void TickClock()
{
    if ((FrameTick % (bPALSystem?50:60)) == 0)
    {
        SystemUptime++;
        TimeSync++;
 
        // 1.   0 adjust: Over 8.5 hours clock has drifted -34 seconds, which is 4 seconds per hour or 1 second per 15 minutes (900 seconds)
        // 2. 890 adjust: Over 10.5 hours clock has drifted -8 seconds, which is 0.75 secondsd per hour or 3 seconds ever 4 hours
        // 3. 650 adjust: Idle at prompt; Too fast - 2-3 seconds over 2 hours too fast
        // Todo: Check 50 hz - above is for 60 hz
        // Leap frame :^)
        if ((TimeSync % (900-11)) == 0)
        {
            SystemUptime++;
            TimeSync++;
        }

        SecondsTo_HHMM(&SystemTime, TimeSync);
    }
    
    FrameTick++;
}

void SetSystemDateTime(u32 seconds)
{
    TimeSync = seconds + (sv_TimeZone * 3600);
    LastTimeSync = TimeSync;
    SecondsToDateTime(&SystemTime, TimeSync);
}

u32 GetTimeSinceLastSync()
{
    return (TimeSync - LastTimeSync);
}

u32 GetTimeSync()
{
    return TimeSync;
}

u8 DoTimeSync(char *server)
{
    u8 data;
    u32 recv = 0;
    u8 i = 0;
    u32 timeout = 0;
    char *sync_server;

    if ((SystemUptime > 9) && (GetTimeSinceLastSync() < 10))
    {
        return 2;   // Don't sync. Time was synchronized within the last 10 seconds.
    }

    if (server != NULL) sync_server = server;
    else sync_server = sv_TimeServer;

    Buffer_Flush0(&RxBuffer);

    if (NET_Connect(sync_server))
    {
        while (Buffer_GetNum(&RxBuffer) < 4)
        {
            if (timeout++ >= 50)
            {
                //printf("DEBUG: Timeout waiting for buffer>=4 (buf_num == %u)\n", Buffer_GetNum(&RxBuffer));
                NET_Disconnect(); 
                return 1;
            }
            waitMs(100);
        }

        //printf("DEBUG: Time taken waiting for buffer to fill: %lu", timeout);
        timeout = 0;

        while (Buffer_Pop(&RxBuffer, &data))
        {
            recv = (recv << 8) + data;

            if (i++ >= 3) break;
            
            if (timeout++ >= 10000)
            {
                //printf("DEBUG: Timeout waiting for i>=3 (i == %u)\n", i);
                NET_Disconnect(); 
                return 1;
            }
        }

        SetSystemDateTime(recv);

        NET_Disconnect(); 

        return 0;
    }

    return 1;   // Connection to <server> failed
}
