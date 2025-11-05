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
#define ICO_MOUSE_OK   0x20 // Mouse input device 'M'

typedef enum 
{
    DP_None  = 0, 
    DP_Port1 = 1, 
    DP_Port2 = 2, 
    DP_Port3 = 3, 
    DP_CART  = 4, 
    DP_EXP   = 5
} DevPort;

typedef struct
{
    char *sName;    // Device name
    u8 Bitmask;     // Used bits        (Example; Used bits = b00000011)
    u8 Bitshift;    // Used bits shift  (Example; If Shift=2 then Used bits = b00001100)
    u8 Mode;        // 1=Parallel / 2=Serial / 3=Both
} SM_DeviceId;

typedef struct
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

#define DEV_MASK_SHIFT(d) (d.Id.Bitmask << d.Id.Bitshift)                                           // Helper macro to get the shifted mask
#define DEV_MASK_AND_SHIFT(d, b) ((b & d.Id.Bitmask) << d.Id.Bitshift)                              // Helper macro to apply the mask and shift to a value 'b'

#define DEV_SetCtrl(d, b) (*d.Ctrl =  (*d.Ctrl & ~DEV_MASK_SHIFT(d)) | DEV_MASK_AND_SHIFT(d, b))    // Set control byte (set pin direction), d = SM_Device, b = byte (0 = input, 1 = output)
#define DEV_ClrCtrl(d)    (*d.Ctrl &= ~DEV_MASK_SHIFT(d))                                           // Clear control byte (set pin direction to input for masked bits), d = SM_Device

#define DEV_SetData(d, b) (*d.Data =  (*d.Ctrl & ~DEV_MASK_SHIFT(d)) | DEV_MASK_AND_SHIFT(d, b))    // Set data byte (pin output), d = SM_Device, b = byte
#define DEV_ClrData(d)    (*d.Data &= ~DEV_MASK_SHIFT(d))                                           // Clear data byte (set pin output low), d = SM_Device
#define DEV_GetData(d, b) (*d.Data & DEV_MASK_AND_SHIFT(d, b) >> d.Id.Bitshift)                     // Get data byte (read pin input), d = SM_Device, b = mask for bits of interest

extern SM_Device *DevList[DEV_MAX];
extern DevPort sv_ListenPort;   // Default UART port to listen on
extern bool bRLNetwork;
extern bool bXPNetwork;
extern bool bMegaCD;
extern bool bVRAM_128KB;
extern bool bMouse;

void SetDevicePort(SM_Device *d, DevPort p);
void DeviceManager_Init();

#endif // DEVMGR_H_INCLUDED
