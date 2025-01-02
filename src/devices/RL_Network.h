#include <genesis.h>

/*
MIT License

Copyright (c) 2023 B1tsh1ft3r

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

Modified for use in SMDT by smds.
Originial source at: https://github.com/b1tsh1ft3r/retro.link/tree/main/sega_genesis/sgdk_example
*/

#define UART_BASE   0xA130C1
#define UART_RHR    (*((volatile uint8_t*)(UART_BASE+0)))  // Receive holding register
#define UART_THR    (*((volatile uint8_t*)(UART_BASE+0)))  // Transmit holding register
#define UART_IER    (*((volatile uint8_t*)(UART_BASE+2)))  // Interrupt enable register
#define UART_FCR    (*((volatile uint8_t*)(UART_BASE+4)))  // FIFO control register
#define UART_LCR    (*((volatile uint8_t*)(UART_BASE+6)))  // Line control register
#define UART_MCR    (*((volatile uint8_t*)(UART_BASE+8)))  // Modem control register
#define UART_LSR    (*((volatile uint8_t*)(UART_BASE+10))) // Line status register
#define UART_DLL    (*((volatile uint8_t*)(UART_BASE+0)))  // Divisor latch LSB. Acessed only when LCR[7] = 1
#define UART_DLM    (*((volatile uint8_t*)(UART_BASE+2)))  // Divisor latch MSB. Acessed only when LCR[7] = 1
#define UART_DVID   (*((volatile uint8_t*)(UART_BASE+2)))  // Device ID 

#define RL_REPORT_BAUD "9600"

extern u8 sv_DLM;
extern u8 sv_DLL;

u8   RLN_ReadByte(void);

bool RLN_Connect(char *str);
bool RLN_Initialize(void);
bool RLN_TXReady();
bool RLN_RXReady();

void RLN_SendByte(u8 data);
void RLN_SendMessage(char *str);

void RLN_FlushBuffers(void);

void RLN_EnterMonitorMode(void);
void RLN_ExitMonitorMode(void);
void RLN_AllowConnections(void);
void RLN_BlockConnections(void);
void RLN_ResetAdapter(void);
void RLN_Update(void);

u8 RLN_GetIP(char *ret);
void RLN_PrintIP(int x, int y);
void RLN_PrintMAC(int x, int y);
void RLN_PingIP(char *ip);