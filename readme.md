
# SMD Terminal emulator, Telnet and IRC client v0.29+
A terminal emulator, telnet and IRC client for the Sega Mega Drive/Genesis with support for PS/2 keyboards and RS-232 communication.<br>
![Screenshot of the telnet client](https://deceptsoft.com/smdtc_extra_git/telnet_small.png)
![Screenshot of the IRC client](https://deceptsoft.com/smdtc_extra_git/irc_small.png)
![Screenshot of the terminal emulator showing nano](https://deceptsoft.com/smdtc_extra_git/blastem_20240401_104314.png)
![Screenshot of a debugging utility to inspect streams](https://deceptsoft.com/smdtc_extra_git/hexview_small.png)
![Screenshot of the telnet client in 80 column + 8 colour mode](https://deceptsoft.com/smdtc_extra_git/blastem_20240401_203819.png)
![Screenshot of the terminal emulator](https://deceptsoft.com/smdtc_extra_git/blastem_20240505_222454.png)

Extra hardware:<br>
A PS/2 keyboard or a Sega Saturn keyboard (not required but preferred).<br>
A "voltage translator" to translate between the MD +5v and remote RS232 device logic levels,<br>
I personally recommend the max3232 (https://www.ti.com/lit/ds/symlink/max3232.pdf)<br>
<br>
xPico/RetroLink support as an alternative to the above is being worked on.<br>
<br>

## Disclaimer
> [!WARNING]
> **Do note that the MD is a 5 volt system!** That means you should take care to not connect any random serial device directly to the MD (Such as a PC).<br>
> Use a "voltage translator" such as the max3232 between your MD and remote device to translate the voltage levels.<br>
> Make sure you understand my ramble down under the "Device" section to hook up external devices correctly!<br>
> I (smds) will not take any responsibilities for any failure to read and understand the above warning.<br>
<br>

## Thanks to
b1tsh1ft3r - Testing, improvement ideas and RetroLink/xPico support<br>
RKT - For creating a 4x8 extended ASCII font tileset<br>
Stef - For creating [SGDK](https://github.com/Stephane-D/SGDK)<br>
<br>

## Building SMDTC from source
**This part needs to be expanded, for now it assumes you are familiar with SGDK and how to use it.**<br><br>
 To build SMDTC from source you will need SGDK version 1.80 (newer versions untested but will probably work as SMDTC mostly only uses macros and basic functions from SGDK)<br>
The SGDK library must be rebuilt with the flags `HALT_Z80_ON_IO` and `HALT_Z80_ON_DMA` set to 0 in config.h to make sure the z80 CPU is never getting its bus back.<br>
<br>

## Devices
SMDTC has a device manager which can autodetect if a device is present and where it is plugged in.<br>
Device detection is only done on bootup, no plug & play support (yet).<br>
<br>
A total of 2 PS/2 devices and 1 UART device can potentially be connected to a single MD controller port. However, beware the power draw may exceed what the MD can supply!<br>
<br>
When a keyboard is connected and detected a 'K' icon will be visible in the status bar.<br>
A fallback joypad device will be activated if SMDTC fails to find a keyboard or when a keyboard is plugged into PORT 2, allowing the use of a regular joypad.<br>
<br>
All detected devices can be viewed in the "Connected devices" list (Quick menu -> Mega Drive settings -> Connected devices)<br>
<br>
> [!NOTE]
> SMDTC is limited when it comes to detecting the presence of a serial connection on the built in UART (Only an xPico module can be autodetected)<br>
> By default SMDTC will listen for incoming connections on PORT 2 UART.<br>
> This setting can be changed in the "Select serial port" menu (Quick menu -> Mega Drive settings -> Select serial port)<br>
> Do not forget to save your changes! (Quick menu -> Reset -> Save config to sram)<br>
<br>

### List of autodetected devices that require no configuration
PS/2 Keyboard.<br>
Sega Saturn keyboard.<br>
Sega 3/6 button joypad.<br>
xPico module connected to built in UART (Currently being worked on, support may be iffy)<br>
RetroLink network adapter cartridge  (Currently being worked on, support may be iffy)<br>
<br>

### How to wire up a PS/2 keyboard
For PS/2 devices (a keyboard or mouse) SMDTC has two ways to connect one:<br>

Example keyboard wiring (1):<br>
MD port pin 1 = PS/2 clock pin 5<br>
MD port pin 2 = PS/2 data pin 1<br>
MD port pin 5 = PS/2 VCC pin 4<br>
MD port pin 8 = PS/2 GND pin 3<br>

Example keyboard wiring (2):<br>
MD port pin 3 = PS/2 clock pin 5<br>
MD port pin 4 = PS/2 data pin 1<br>
MD port pin 5 = PS/2 VCC pin 4<br>
MD port pin 8 = PS/2 GND pin 3<br>
<br>
See the pin configuration lists below for pinouts of the MD controller ports and PS/2 keyboard.<br>
<br>

### Pin configuration of the MD controller ports: 
MD port pin 1 = PS/2 device 1 clock  (CLK1)<br>
MD port pin 2 = PS/2 device 1 data   (DATA1)<br>
MD port pin 3 = PS/2 device 2 clock  (CLK2)<br>
MD port pin 4 = PS/2 device 2 data   (DATA2)<br>
MD port pin 5 = VCC (+5V)<br>
MD port pin 6 = Serial TX<br>
MD port pin 7 = Reserved             (CP3)<br>
MD port pin 8 = GND<br>
MD port pin 9 = Serial RX<br>

### PS/2 pin reference:
PS/2 pin 1 = Data<br>
PS/2 pin 3 = GND<br>
PS/2 pin 4 = VCC<br>
PS/2 pin 5 = Clock<br>

### MD UART pin reference:
MD port pin 5 = VCC<br>
MD port pin 6 = TX<br>
MD port pin 8 = GND<br>
MD port pin 9 = RX<br>

### Connected device list:
P1:0 = Port 1 @ pin 1+2<br>
P1:1 = Port 1 @ pin 3+4<br>
P1:S = Port 1 UART<br>
P1:D = Port 1 Parallel+UART Mode<br>
P2:0 = Port 2 @ pin 1+2<br>
P2:1 = Port 2 @ pin 3+4<br>
P2:S = Port 2 UART<br>
P2:D = Port 2 Parallel+UART Mode<br>
P3:0 = Port 3 @ pin 1+2<br>
P3:1 = Port 3 @ pin 3+4<br>
P3:S = Port 3 UART<br>
P3:D = Port 3 Parallel+UART Mode<br>

## Quick menu shortcuts
Right windows key OR F8 = Open the Quick menu<br>
Enter = Enter submenu and activate a choice<br>
Escape = Back out of current menu<br>
Up cursor = Move selector up<br>
Down cursor = Move selector down<br>

## IRC client shortcuts
F1 = Channel 1 tab<br>
F2 = Channel 2 tab<br>
F3 = Channel 3 tab<br>
F5 = Toggle channel user list<br>
Numpad 4 = Scroll left<br>
Numpad 6 = Scroll right<br>

## Telnet client shortcuts
F11 = Send ^C to remote server<br>
F12 = Send ^X to remote server<br>
Numpad 1 = Scroll left<br>
Numpad 3 = Scroll right<br>

## Terminal emulator
Type `help` for a list of all available built in commands.<br>
Up arrow   = Retype last entered string<br>
Down arrow = Clear current typed string<br>
