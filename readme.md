
# SMD Terminal emulator, Telnet and IRC client v0.31+
A terminal emulator, telnet and IRC client for the Sega Mega Drive/Genesis with support for keyboards and RS-232 communication.<br>
![Screenshot of the telnet client](https://deceptsoft.com/smdtc_extra_git/v30/telnet_small.png)
![Screenshot of the IRC client](https://deceptsoft.com/smdtc_extra_git/v30/irc_small.png)
![Screenshot of the terminal emulator showing nano](https://deceptsoft.com/smdtc_extra_git/v30/blastem_20240401_104314.png)
![Screenshot of a debugging utility to inspect streams](https://deceptsoft.com/smdtc_extra_git/v30/hexview_small.png)
![Screenshot of the telnet client in 80 column + 8 colour mode](https://deceptsoft.com/smdtc_extra_git/v30/blastem_20240401_203819.png)
![Screenshot of the terminal emulator](https://deceptsoft.com/smdtc_extra_git/v30/blastem_20240505_222454.png)

##### Table of Contents
* [Disclaimer](#disclaimer)
* [Thanks](#thanks-to)
* [Building from source](#building-smdtc-from-source)
* [Running SMDTC](#running-smdtc)
* [Required hardware](#required-hardware)
* [Devices](#devices)
  * [Autodetected devices](#list-of-autodetected-devices)
  * [How to wire up a PS/2 keyboard](#how-to-wire-up-a-ps2-keyboard)
  * [Pin configuration of the MD controller ports](#pin-configuration-of-the-md-controller-ports)
  * [PS/2 pin reference](#ps2-pin-reference)
  * [MD UART pin reference](#md-uart-pin-reference)
* [Filesystem](#filesystem)
* [Shortcuts](#shortcuts)
  * [Quick menu](#quick-menu)
  * [IRC client](#irc-client)
  * [Telnet client](#telnet-client)
  * [Gopher client](#gopher-client)
  * [Terminal emulator](#terminal-emulator)

## Disclaimer
> [!WARNING]
> **Do note that the MD is a 5 volt system!** That means you should take care to not connect any random serial device directly to the MD (Such as a PC).<br>
> Use a "voltage translator" such as the max3232 between your MD and remote device to translate the voltage levels.<br>
> Make sure you understand my ramble down under the "Device" section to hook up external devices correctly!<br>
> I (smds) will not take any responsibilities for any failure to read and understand the above warning.<br>
<br>

## Thanks to
b1tsh1ft3r - Testing, improvement ideas and RetroLink/xPort support<br>
RKT - For creating a 4x8 extended ASCII font tileset<br>
Stef - For creating [SGDK](https://github.com/Stephane-D/SGDK)<br>
Sik - For creating the website [Plutiedev](https://plutiedev.com/) with valuable information regarding the MD<br>
[littlefs](https://github.com/littlefs-project/littlefs) - For an awesome little filesystem<br>
<br>

## Building SMDTC from source
**This part needs to be expanded, for now it assumes you are familiar with SGDK and how to use it.**<br><br>
 To build SMDTC from source you will need SGDK version 1.80 (newer versions untested but will probably work as SMDTC mostly only uses macros and basic functions from SGDK)<br>
The SGDK library must be rebuilt with the flags `HALT_Z80_ON_IO` and `HALT_Z80_ON_DMA` set to 0 in config.h to make sure the z80 CPU is never getting its bus back.<br>
<br>

## Running SMDTC
SMDTC is made to run on the original Mega Drive / Genesis hardware;<br>
Easiest way to run SMDTC on your system is by transferring the binary file `smdt_vX.YY.Z.bin` to a flashcart.<br>
<br>
You can also run SMDTC in a Mega Drive / Genesis emulator and easily check it out; Do mind that most emulators do not provide any way to actually connect any external serial devices, so no network support is possible while running in an emulator.<br><br>
I highly recommend the emulator [BlastEm](https://www.retrodev.com/blastem/nightlies/) since it supports the Sega Saturn keyboard.<br>
Other emulators may have issues running SMDTC and it is unlikely they will have any keyboard support.
<br>

## Required hardware
1. A PS/2 keyboard or a Sega Saturn keyboard (not strictly required but preferred).
2. A **5 volt** RS-232 serial connection or an xPort/xPort module.
3. A Mega Drive or Genesis and a way to run roms on it.
<br>

RetroLink network cartridge as an alternative network adapter is being worked on.<br>

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
> SMDTC is limited when it comes to detecting the presence of a serial connection on the built in UART.<br>
> By default SMDTC will listen for incoming connections and attempt to find serial devices on PORT 2 UART.<br>
> This setting can be changed in the "Select serial port" menu (Quick menu -> Mega Drive settings -> Select serial port)<br>
> Do not forget to save your changes! (Quick menu -> Reset -> Save config to sram)<br>
<br>

### List of autodetected devices
PS/2 Keyboard.<br>
Sega Saturn keyboard.<br>
Sega 3/6 button joypad.<br>
xPort module connected to built in UART (May require you to set the correct serial port as described above)<br>
RetroLink network adapter cartridge (Currently being worked on, support may be iffy)<br>
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

### Pin configuration of the MD controller ports
MD port pin 1 = PS/2 device 1 clock  (CLK1)<br>
MD port pin 2 = PS/2 device 1 data   (DATA1)<br>
MD port pin 3 = PS/2 device 2 clock  (CLK2)<br>
MD port pin 4 = PS/2 device 2 data   (DATA2)<br>
MD port pin 5 = VCC (+5V)<br>
MD port pin 6 = Serial TX<br>
MD port pin 7 = Reserved             (CP3)<br>
MD port pin 8 = GND<br>
MD port pin 9 = Serial RX<br>

### PS/2 pin reference
PS/2 pin 1 = Data<br>
PS/2 pin 3 = GND<br>
PS/2 pin 4 = VCC<br>
PS/2 pin 5 = Clock<br>

### MD UART pin reference
MD port pin 5 = VCC<br>
MD port pin 6 = TX<br>
MD port pin 8 = GND<br>
MD port pin 9 = RX<br>

### Connected device list
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

## Filesystem
SMDT features a [littlefs](https://github.com/littlefs-project/littlefs) formatted filesystem that lives in SRAM.<br>
It is currently hardcoded to a 32KB partition and SRAM is configured as BYTE/ODD. Maybe I'll add support for flashcart SD cards in the future?<br>
SMDT will still work without SRAM but with no possibility to save settings due to obvious reasons.<br>
The filesystem can be navigated and manipulated in the terminal using basic UNIX commands such as: ls, mv, cp, cat, touch, mkdir, rm, lsblk etc etc.<br>
<br>
This is still very much WIP, beware data loss and bugs!<br>
<br>
The little filesystem<br>
Copyright (c) 2022, The littlefs authors.<br>
Copyright (c) 2017, Arm Limited. All rights reserved.<br>
[See littlefs on github](https://github.com/littlefs-project/littlefs)<br>

## Shortcuts

### Quick menu
Right windows key OR F8 = Open the Quick menu<br>
Enter = Enter submenu and activate a choice<br>
Escape = Back out of current menu<br>
Up cursor = Move selector up<br>
Down cursor = Move selector down<br>

### IRC client
F1  = Channel 1 tab<br>
F2  = Channel 2 tab<br>
F3  = Channel 3 tab<br>
F4  = Channel 4 tab<br>
F5  = Channel 5 tab<br>
Left arrow key  = Switch to previous channel<br>
Right arrow key = Switch to next channel<br>
Tab = Toggle channel user list<br>
Numpad 4 = Scroll left<br>
Numpad 6 = Scroll right<br>

### Telnet client
None<br>

### Gopher client
Very WIP gopher client, use at your own risk!<br>
Cursor keys = Move the cursor on screen<br>
Enter       = Go to link highlighted by the cursor<br>

### Terminal emulator
Type `help` for a list of all available built in commands.<br>
<br>History queue:<br>
Up arrow   = Go back in history of entered command strings.<br>
Down arrow = Go forward in history or clear command string if at the last entered command string.<br>
