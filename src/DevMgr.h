#ifndef DEVMGR_H_INCLUDED
#define DEVMGR_H_INCLUDED

#include <genesis.h>

// Port Addresses
#define PORT1_DATA  0xA10003
#define PORT1_CTRL  0xA10009
#define PORT1_SCTRL 0xA10013
#define PORT1_SRx   0xA10011
#define PORT1_STx   0xA1000F

#define PORT2_DATA  0xA10005
#define PORT2_CTRL  0xA1000B
#define PORT2_SCTRL 0xA10019
#define PORT2_SRx   0xA10017
#define PORT2_STx   0xA10015

#define PORT3_DATA  0xA10007
#define PORT3_CTRL  0xA1000D
#define PORT3_SCTRL 0xA1001F
#define PORT3_SRx   0xA1001D
#define PORT3_STx   0xA1001B

#define DEVMODE_PARALLEL 1
#define DEVMODE_SERIAL   2

#define ICO_ID_UNKNOWN 0x1F // Unknown input device '?'
#define ICO_KB_OK      0x1C // Keyboard input device 'K'
#define ICO_JP_OK      0x1E // Joypad input device 'J'
#define ICO_ID_ERROR   0x1D // Error with input device 'X'

typedef enum e_port {DP_None = 0, DP_Port1 = 1, DP_Port2 = 2, DP_Port3 = 3} DevPort;

typedef struct s_deviceid
{
    char *sName;    // Device name
    u8 Bitmask;     // Used bits        (Example; Used bits = b00000011)
    u8 Bitshift;    // Used bits shift  (Example; If Shift=2 then Used bits = b00001100)
    u8 Mode;        // 1=Parallel / 2=Serial / 3=Both
} SM_DeviceId;

typedef struct s_device
{
    vu8 *Ctrl;      // Control port
    vu8 *Data;      // Data port
    vu8 *SCtrl;     // Serial control port
    vu8 *RxData;    // Rx port
    vu8 *TxData;    // Tx port
    DevPort PAssign;// Port assignment
    SM_DeviceId Id;
} SM_Device;

#define DEV_MAX 6

#define DEV_PORT ((s >> 1)+1)
#define DEV_SLOT(device) (device.Id.Bitshift >> 1)
#define DEV_FULL(device) DEV_PORT, DEV_SLOT(device)

#define DEV_SetCtrl(d, b) (*d.Ctrl  =  (*d.Ctrl & ~(d.Id.Bitmask << d.Id.Bitshift)) | ((b & d.Id.Bitmask) << d.Id.Bitshift))
#define DEV_ClrCtrl(d)    (*d.Ctrl &= ~(d.Id.Bitmask << d.Id.Bitshift))

#define DEV_SetData(d, b) (*d.Data  =  (*d.Ctrl & ~(d.Id.Bitmask << d.Id.Bitshift)) | ((b & d.Id.Bitmask) << d.Id.Bitshift))
#define DEV_ClrData(d)    (*d.Data &= ~(d.Id.Bitmask << d.Id.Bitshift))
#define DEV_GetData(d, b) (*d.Data & ((b & d.Id.Bitmask) << d.Id.Bitshift) >> d.Id.Bitshift)

extern SM_Device *DevList[DEV_MAX];
extern DevPort sv_ListenPort;   // Default UART port to listen on
extern bool bRLNetwork;
extern bool bXPNetwork;

void SetDevicePort(SM_Device *d, DevPort p);
void DeviceManager_Init();

#endif // DEVMGR_H_INCLUDED
