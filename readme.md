
# SMD Telnet/IRC/Terminal client v0.29+
A telnet, IRC and terminal client for the Sega Mega Drive/Genesis with support for PS/2 keyboards and RS-232 communication.<br>
![Screenshot of the telnet client](https://deceptsoft.com/smdtc_extra_git/telnet_small.png)
![Screenshot of the IRC client](https://deceptsoft.com/smdtc_extra_git/irc_small.png)
![Screenshot of the terminal emulator showing nano](https://deceptsoft.com/smdtc_extra_git/blastem_20240401_104314.png)
![Screenshot of a debugging utility to inspect streams](https://deceptsoft.com/smdtc_extra_git/hexview_small.png)
![Screenshot of the telnet client in 80 column + 8 colour mode](https://deceptsoft.com/smdtc_extra_git/blastem_20240401_203819.png)

Extra hardware:<br>
1. PS/2 keyboard or a Sega Saturn keyboard (not required but preferred).<br>
2. A "voltage translator" to translate between the MD +5v and remote RS232 device logic levels,<br>
I personally recommend the max3232 (https://www.ti.com/lit/ds/symlink/max3232.pdf)<br>
<br>

## Disclaimer - READ ME OR FRY YOUR SYSTEM
> [!CAUTION]
**Do note that the MD is a 5 volt system!** That means you should take care to not connect any random serial device directly to the MD (Such as a PC).<br>
Use a "voltage translator" such as the max3232 between your MD and remote device to translate the voltage levels.<br>
Make sure you understand my ramble down under the "Device" section to hook up external devices correctly!<br>
I (smds) will not take any responsibilities for any failure to read and understand the above warning.<br>
<br>

## Credits
b1tsh1ft3r - Testing, improvement ideas and RetroLink/xPico support<br>
RKT - For creating a 4x8 extended ASCII font tileset<br>
smds - Code<br>
<br>

## Building SMDTC from source
**This part needs to be expanded, for now it assumes you are familiar with SGDK and how to use it.**<br>
To build SMDTC from source you will need SGDK version 1.80 (newer versions untested but will probably work as SMDTC does not use very much from it)<br>
The SGDK library must be rebuilt with the flags `HALT_Z80_ON_IO` and `HALT_Z80_ON_DMA` set to 0 in config.h to make sure the z80 CPU is never getting its bus back.<br>
<br>

## Devices
SMDTC has a device manager which can detect certain devices to a degree. PS/2 devices such as a keyboard is one such device that can be detected.<br>
Device detection is only done on bootup, no plug & play support (yet).<br>
<br>
A total of 2 PS/2 devices and 1 UART device can be connected to a single MD controller port.<br>
<br>
When a keyboard is connected and detected a 'K' icon will be visible in the status bar.<br>
A fallback joypad device will be activated if SMDTC fails to find a keyboard or when a keyboard is plugged into PORT 2, allowing the use of a regular joypad.<br>
However a keyboard is recommended to be able to actually type or use special functions.<br>
<br>
TODO: Insert information about using a retrolink or xPico adapter with SMDTC here.
<br>
All detected devices can be viewed by going to the quick menu -> mega drive settings -> connected devices.<br>
<br>

> [!IMPORTANT]
Devices are expected to be connected in a certain way to the MD controller ports, see the pin configuration list below.<br>
PS/2 device pins (clock and data) are expected to be in a pair, for example:<br>
If a keyboard use pin 1 for clock then pin 2 must be used for data.<br>
If a keyboard use pin 3 for clock then pin 4 must be used for data.<br>
Do not swap clock and data pins.<br>
<br>

> [!NOTE]
SMDTC cannot detect the presence of a serial connection (UART), by default SMDTC will listen on PORT 2. This setting can be changed in the quick menu -> mega drive settings -> select serial port.<br>
Do not forget to save your changes by going to the quick menu -> reset -> save config to sram.<br>
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
<br>

### Example keyboard wiring:
MD port pin 1 = PS/2 clock pin 5<br>
MD port pin 2 = PS/2 data pin 1<br>
MD port pin 5 = PS/2 VCC pin 4<br>
MD port pin 8 = PS/2 GND pin 3<br>
<br>

### PS/2 pin reference:
PS/2 pin 1 = Data<br>
PS/2 pin 3 = GND<br>
PS/2 pin 4 = VCC<br>
PS/2 pin 5 = Clock<br>
<br>

### MD UART pin reference:
MD port pin 5 = VCC<br>
MD port pin 6 = TX<br>
MD port pin 8 = GND<br>
MD port pin 9 = RX<br>
<br>

### Connected device list:
P1:0 = Port 1 @ pin 1+2<br>
P1:1 = Port 1 @ pin 3+4<br>
P1:S = Port 1 UART<br>
P2:0 = Port 2 @ pin 1+2<br>
P2:1 = Port 2 @ pin 3+4<br>
P2:S = Port 2 UART<br>
P3:0 = Port 3 @ pin 1+2<br>
P3:1 = Port 3 @ pin 3+4<br>
P3:S = Port 3 UART<br>
<br>

## PS/2 Keyboard shortcuts
Right windows key OR F8 = Open the Quick menu<br>
Enter = Enter submenu and activate a choice<br>
Escape = Back out of current menu<br>
Up cursor = Move selector up<br>
Down cursor = Move selector down<br>

## IRC Client shortcuts
F1 = Channel 1 tab
F2 = Channel 2 tab
F3 = Channel 3 tab
F5 = Toggle channel user list

## Terminal emulator
Type `help` for a list of all available built in commands.
Up arrow   = Retype last entered string
Down arrow = Clear current typed string
