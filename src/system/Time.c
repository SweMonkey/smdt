#include "Time.h"
#include "Utils.h"
#include "Network.h"

// https://www.epochconverter.com/

SM_Time SystemTime;
s32 SystemUptime = 0;            // How long the system has been running (Seconds)

static s32 TimeSync = 0;         // Synchronized time (Seconds)
static s32 LastTimeSync = -999;  // Time when last synchronized (Seconds)
static u32 FrameTick = 0;        // 1/50(PAL) or 1/60(NTSC) ticker

s8 sv_TimeZone = 2;              // Time zone - UTC -12 +14
char sv_TimeServer[32] = "time.nist.gov:37"; // Time sync server
u16 sv_EpochStart = 1970;


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

    t.year  += sv_EpochStart;
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
    if ((FrameTick % (bPALSystem?50:60)) == 0)
    {
        SystemUptime++;
        TimeSync++;

        //FrameTick++;    // Leap frame :^)

        SystemTime = SecondsToDateTime(TimeSync);
    }
    
    FrameTick++;
}

void SetSystemDateTime(s32 seconds)
{
    TimeSync = seconds + (sv_TimeZone * 3600);
    LastTimeSync = TimeSync;
    SystemTime = SecondsToDateTime(TimeSync);
}

s32 GetTimeSinceLastSync()
{
    return (TimeSync - LastTimeSync);
}

u8 DoTimeSync(char *server)
{
    u8 data;
    s32 recv = 0;
    u8 i = 0;
    u32 timeout = 0;
    char *sync_server;

    if (GetTimeSinceLastSync() < 10)
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
            if (timeout++ >= 100000)
            {
                NET_Disconnect(); 
                return 1;
            }
        }

        timeout = 0;

        while (Buffer_Pop(&RxBuffer, &data) != 0xFF)
        {
            recv = (recv << 8) + data;

            if (i++ >= 3) break;
            
            if (timeout++ >= 100000)
            {
                NET_Disconnect(); 
                return 1;
            }
        }

        // SMDTC assumes unix time (start year 1970), the received time however may use 1900 as starting year... fixme?
        SetSystemDateTime(recv);

        NET_Disconnect(); 

        return 0;
    }

    return 1;   // Connection to <server> failed
}
