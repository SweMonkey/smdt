#include "HTTP_Webserver.h"
#include "Keyboard.h"
#include "Input.h"
#include "Network.h"
#include "Utils.h"
#include "StateCtrl.h"
#include "devices/XP_Network.h"
#include "system/Time.h"
#include "system/File.h"
#include "system/PseudoFile.h"

#define DATA_BUFFER_SIZE 1024

static u8 rxdata;
static u8 *buffer = NULL;
static u8 old_echo;
static u16 buffer_it = 0;
static char *request_src;

// External
extern u8 vDoEcho;
void ShellDrawClockUpdate();

//static const char *dummy_request = "DCI192.168.10.1\r\n\
GET /index.html HTTP/1.1\r\n\
Host: 192.168.10.10:5364\r\n\
User-Agent: Mozilla/5.0 (X11; linux x86_64; rv:140.0) Gecko/20100101 Firefox/140.0\r\n\
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n\
Accept-language: en-US,en;q=0.5\r\n\
Accept-Encoding: gzip, deflate\r\n\
DNT: 1\r\n\
Connection: keep-alive\r\n\
Upgrade-Insecure-Requests: 1\r\n\
Priority: u=0, i\r\n\r\n";


static void ServeClose()
{
    // Set CP3 pin to tell the xPort to disconnect from the remote server
    DEV_SetData(DRV_UART, 0x40);// Set pin 7 high
    waitMs(100);
    DEV_ClrData(DRV_UART);      // Set pin 7 low
}

static void ServeEnd()
{
    NET_SendChar(0xD);
    NET_SendChar(0xA);
    NET_SendChar(0xD);
    NET_SendChar(0xA);
    
    waitMs(100);

    ServeClose();
}

static void ServeFail()
{
    NET_SendString("HTTP/1.1 200 OK\r\n");
    NET_SendString("Content-Type: text/html; charset=UTF-8\r\n");
    NET_SendString("Content-Length: 30\r\n\r\n");
    NET_SendString("<h1>Internal server error</h1>");
    ServeEnd();
}

static void ServeRequest(const char *filename)
{
    char *str1 = "HTTP/1.1 200 OK\r\n";
    char *str2 = "Content-Type: text/html; charset=UTF-8\r\n";
    char str3[48];      // Date
    char str4[56];      // Last modified
    char str5[32];      // Content length
    char *str6 = NULL;  // Content

    // Date
    snprintf(str3, 48, "Date: %s, %u %s %u %u:%u:%u %s\r\n", "Fri", SystemTime.day, "Sep", SystemTime.year, SystemTime.hour, SystemTime.minute, SystemTime.second, "GMT");
    // Last modified
    snprintf(str4, 56, "Last-Modified: Fri, 19 Sep 2025 00:00:00 GMT\r\n");

    // Open/Load content file
    char fn_buf[FILE_MAX_FNBUF];
    SM_File *f = NULL;

    FS_ResolvePath(filename, fn_buf);
    
    f = F_Open(fn_buf, FM_RDONLY);

    if (f == NULL)
    {
        printf("File \"%s\" does not exist\n", filename);
        goto OnExit;
    }

    F_Seek(f, 0, SEEK_END);
    u16 size = F_Tell(f);

    str6 = (char*)malloc(size);

    if (str6 == NULL) goto OnExit;

    memset(str6, 0, size);
    F_Seek(f, 0, SEEK_SET);
    F_Read(str6, size, 1, f);

    // Content length
    snprintf(str5, 32, "Content-Length: %u\r\n\r\n", size);

    // Send content to open connection
    NET_SendString(str1);
    NET_SendString(str2);
    NET_SendString(str3);
    NET_SendString(str4);
    NET_SendString(str5);
    NET_SendString(str6);

    ServeEnd();

    if (request_src[0] == 0) printf("Served page \"%s\"\r\n", filename);
    else printf("Served page \"%s\" to %s\r\n", filename, request_src);

    F_Close(f);
    free(str6);

    Stdout_Flush();
    return;

    OnExit:
    F_Close(f);
    free(str6);

    ServeFail();
    Stdout_Flush();
    return;
}

int Feed_crlfcrlf(u8 *matched, u8 c) 
{
    static const char target[4] = { '\r', '\n', '\r', '\n' };

    if (c == target[*matched]) 
    {
        (*matched)++;
        if (*matched == 4) 
        {
            *matched = 0;
            return 1; // match found
        }
    } 
    else *matched = (c == '\r') ? 1 : 0;

    return 0;
}

static void CaptureData(u8 byte)
{
    buffer[buffer_it] = byte;
    buffer_it++;
}

static void ParseData()
{
    u16 start = 0;
    u16 end = 0;
    u16 cnt = 0;
    char str[128];
    char path[128];

    memset(str, 0, 128);
    memset(path, 0, 128);

    // Keep request_src here, only reset it if we get a new IP

    // Get request source
    cnt = (buffer[0] == 'D' && buffer[1] == 'C' && buffer[2] == 'I') ? 3 : ((buffer[0] == 'C' && buffer[1] == 'I') ? 2 : 0);
    
    if (cnt)
    {
        start = cnt;
        end = cnt;

        while ((buffer[end] != '\r') && (end < 128)){end++;};

        //if (end < 128)
        if (buffer[start] >= '0' && buffer[start] <= '9')
        {
            memset(request_src, 0, 128);
            memcpy(request_src, buffer+start, (end-start));
        }
        // Check for missing DCI, if true then reset parser to beginning for the next parse
        else
        {
            start = 0;
            end = 0;
        }
    }
    else if (buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T')
    {
        goto GET;
    }
    else
    {
        printf("DCI mismatch: %c %c %c\r\n", buffer[0], buffer[1], buffer[2]);
        Stdout_Flush();
        goto Fail;
    }


    // Get requested page
    GET:

    strcat(path, "/sram/www");
    memset(str, 0, 128);

    while (buffer[start] != 'G')
    {
        start++;
        // "Failsafe"
        if (start+5 >= DATA_BUFFER_SIZE) goto Fail;
    }

    if (buffer[start] != 'G' || buffer[start+1] != 'E' || buffer[start+2] != 'T' || buffer[start+3] != ' ') goto Fail;

    start += 4;
    end = start;
    while (buffer[end] != ' ')
    {
        end++;
        // "Failsafe"
        if (end >= DATA_BUFFER_SIZE) goto Fail;
    }

    memcpy(str, buffer+start, (end-start));

    // Return default index.html
    if (strcmp(str, "/") == 0)
    {
        strcat(path, "/index.html");
    }
    // Ignore requests for favicon
    /*else if (strcmp(str, "/favicon.ico") == 0)
    {
        ServeClose();
        return;
    }*/
    // Return requested page
    else
    {
        SM_File *f = NULL;
        char fn_buf[FILE_MAX_FNBUF];
        char tmp[128];
        
        memset(tmp, 0, 128);
        memcpy(tmp, path, 128);
        strcat(tmp, str);

        FS_ResolvePath(tmp, fn_buf);
        f = F_Open(fn_buf, FM_RDONLY);

        if (f == NULL)
        {
            strcat(path, "/404.html");
        }
        else strcat(path, str);

        F_Close(f);
    }

    //kprintf("S: %u - E: %u -- String: \"%s\" -- path: \"%s\" - request_src: \"%s\"", start, end, str, path, request_src);

    ServeRequest(path);
    return;

    Fail:
    ServeFail();
    return;
}

void HTTP_Listen()
{
    bool bHalt = FALSE;
    u8 kbdata = 0;
    u16 it = 0;
    u8 state = 0;

    request_src = malloc(128);
    buffer = malloc(DATA_BUFFER_SIZE);

    if (buffer == NULL)
    {
        printf("Failed to allocate data buffer!\n");
        return;
    }

    old_echo = vDoEcho;
    vDoEcho = 1;

    memset(buffer, 0, DATA_BUFFER_SIZE);

    printf("HTTP Webserver running...\n^c to quit\r\n");
    Stdout_Flush();

    Buffer_Flush(&RxBuffer);
    Buffer_Flush(&TxBuffer);

    // TEMP
    /*u16 len = strlen(dummy_request);
    printf("\n\ndummy request len = %u\n", len);
    Stdout_Flush();
    for (u16 i = 0; i < len; i++)
    {
        Buffer_Push(&RxBuffer, dummy_request[i]);
    }*/
    // TEMP

    while (!bHalt)
    {
        it = 0;

        while (Buffer_Pop(&RxBuffer, &rxdata))
        {
            CaptureData(rxdata);

            if (Feed_crlfcrlf(&state, rxdata)) 
            {
                // Parse and delete old request
                ParseData();
                state = 0;
                buffer_it = 0;
                memset(buffer, 0, DATA_BUFFER_SIZE);
            }
            else if (buffer_it >= 1024)
            {
                // Failsafe, drop request
                state = 0;
                buffer_it = 0;
                memset(buffer, 0, DATA_BUFFER_SIZE);
            }

            if (it++ > 16) break;
        }

        while (KB_Poll(&kbdata)) KB_Interpret_Scancode(kbdata); // Force poll keyboard device

        if (is_KeyDown(KEY_C) && bKB_Ctrl)
        {
            Buffer_Flush(&TxBuffer);    // Clear all data in TxBuffer (may contain user typed characters that we don't want to expose)
            bHalt = TRUE;
        }

        // Fake VBlank update
        VDP_waitVSync();
        VBlank();
        ShellDrawClockUpdate();
    }

    free(request_src);
    free(buffer);
    buffer = NULL;

    vDoEcho = old_echo;
}
