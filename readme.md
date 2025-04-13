
# SMD Terminal emulator, Telnet and IRC client
A terminal emulator, telnet and IRC client for the Sega Mega Drive/Genesis with support for keyboards and RS-232 communication.<br>
![Screenshot of the telnet client in 80 column + 16 colour mode](/doc/images/telnet.png?raw=true)
![Screenshot of the IRC client](/doc/images/irc_02.png?raw=true)
![Screenshot of the terminal emulator showing dwarf fortress](/doc/images/dwarf_fortress.png?raw=true)
![Screenshot of a debugging utility to inspect streams](/doc/images/hexview.png?raw=true)
![Screenshot of the telnet client in 80 column + 8 colour mode](/doc/images/nethack.png?raw=true)
![Screenshot of the terminal emulator](/doc/images/terminal.png?raw=true)

##### Table of Contents
* [Disclaimer](#disclaimer)
* [Thanks](#thanks-to)
* [Building from source](#building-smdt-from-source)
* [Running SMDT](#running-smdt)
  * [Running SMDT in blastEm](#running-smdt-in-blastem)
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
* [FAQ](#faq)

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

## Building SMDT from source
**This part needs to be expanded, for now it assumes you are familiar with SGDK and how to use it.**<br><br>
 To build SMDT from source you will need SGDK version 1.80 (newer versions untested but will probably work as SMDT mostly only uses macros and basic functions from SGDK)<br>
The SGDK library must be rebuilt with the flags `HALT_Z80_ON_IO` and `HALT_Z80_ON_DMA` set to 0 in config.h to make sure the z80 CPU is never getting its bus back.<br>
<br>

## Running SMDT
SMDT is made to run on the original Mega Drive / Genesis hardware;<br>
Easiest way to run SMDT on your system is by transferring the binary file `smdt_vX.YY.Z.bin` to a flashcart.<br>
<br>
You can also run SMDT in a Mega Drive / Genesis emulator and easily check it out; Do mind that most emulators do not provide any way to actually connect any external serial devices or keyboards.<br><br>
I highly recommend the emulator [BlastEm 0.6.3+](https://www.retrodev.com/blastem/nightlies/) since it supports the Sega Saturn keyboard as well as providing the functionality to connect the serial port to a UNIX socket, allowing SMDT to connect to remote servers.<br>
<br>

## Running SMDT in blastEm
To run SMDT on blastEm you need setup IO devices in blastEm, this is partially done by going into the system settings in blastEm (Settings -> System)<br><br>
For IO Port 1 Device, select "Saturn keyboard"<br><br>
To setup IO Port 2 Device you need to navigate to your `blastem.cfg` file, on linux this file is by default located in `~/.config/blastem/`<br>
Open `blastem.cfg` and scroll down to the `io` section.<br>
In the `device` block change `2 <some device>` to `2 serial`<br>
After the `device block`, add this line: `socket smdtsock.sock`<br>

Your `io` section should now look something like this:<br>
```
io {
	devices {
		1 saturn keyboard
		2 serial
		ext none
	}
	socket smdtsock.sock
	ea_multitap {
		1 gamepad6.1
		2 gamepad6.2
		3 gamepad6.3
		4 gamepad6.4
	}
	sega_multitap.1 {
		1 gamepad6.2
		2 gamepad6.3
		3 gamepad6.4
		4 gamepad6.5
	}
}
```
<br>

If the serial port is setup correctly in blastEm, then you can now connect to the above socket by using the tool [SMDT-PC](https://github.com/SweMonkey/smdt-pc) like this:<br>
1. Open `smdt_vX.YY.Z.bin` in blastEm.<br>
2. Launch SMDT-PC with this command in a terminal: `./smdtpc -xportsock /path/to/blastEm/smdtsock.sock`.<br>

<br>
In step 1, blastEm and SMDT will initially halt and wait for a connection being made (step 2)<br>
If all is well SMDT should print out "XPN: xPort module OK" to the boot messages and you're good to go.<br>
<br>

> [!NOTE]
> 1. Requires a very recent nightly build of blastEm as blastEm's serial mode was bugged in versions prior to revision 25e40370e0e4 (2024-10-27).<br>
> 2. SMDT-PC (xPort emulator) is in early stages, it may lack certain features or contain bugs.<br>
> 3. It is not strictly required to use Port 1 and Port 2;<br>
>    SMDT will autodetect which port the keyboard is plugged into.<br>
>    If you want to have the serial port on a different controller port then you will need to change where SMDT looks for it using the Quick Menu (F8): Mega Drive Settings -> Select serial port.<br>

<br>

## Required hardware
1. A PS/2 keyboard or a Sega Saturn keyboard (not strictly required but preferred).
2. A **5 volt** RS-232 serial connection or an xPort module.
3. A Mega Drive or Genesis and a way to run roms on it.
<br>

RetroLink network cartridge as an alternative network adapter is being worked on.<br>

> [!NOTE]
> Many functions in SMDT can be accessed even if you don't have any keyboard input devices, with the obvious exception of anything that requires text input.<br><br>
> Default joypad -> keyboard bindings:<br>
> Joypad A = Keyboard RETURN<br>
> Joypad B = Keyboard ESCAPE<br>
> Joypad Start = Keyboard F8 / Right windows key<br>
> Joypad directionals = Keyboard cursor keys<br>

## Devices
SMDT has a device manager which can autodetect if a device is present and where it is plugged in.<br>
Device detection is only done on bootup, no plug & play support (yet).<br>
<br>
A total of 2 PS/2 devices and 1 UART device can potentially be connected to a single MD controller port. However, beware the power draw may exceed what the MD can supply!<br>
<br>
When a keyboard is connected and detected a 'K' icon will be visible in the status bar.<br>
A fallback joypad device will be activated if SMDT fails to find a keyboard or when a keyboard is plugged into PORT 2, allowing the use of a regular joypad.<br>
<br>
All detected devices can be viewed in the "Connected devices" list (Quick menu -> Mega Drive settings -> Connected devices)<br>
<br>
> [!NOTE]
> SMDT is limited when it comes to detecting the presence of a serial connection on the built in UART.<br>
> By default SMDT will listen for incoming connections and attempt to find serial devices on PORT 2 UART.<br>
> This setting can be changed in the "Select serial port" menu (Quick menu -> Mega Drive settings -> Select serial port)<br>
> Do not forget to save your changes! (Quick menu -> System -> Save config)<br>
<br>

### List of autodetected devices
PS/2 Keyboard.<br>
Sega Saturn keyboard.<br>
Sega 3/6 button joypad.<br>
xPort module connected to built in UART (May require you to set the correct serial port as described above)<br>
RetroLink network adapter cartridge (Currently being worked on, support may be iffy)<br>
<br>

### How to wire up a PS/2 keyboard
For PS/2 devices (a keyboard or mouse) SMDT has two ways to connect one:<br>

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

## FAQ
Q: How do I change the terminal or IRC font?<br>
A: Open the Quick Menu (F8) and go to "Settings -> Default fonts -> Terminal/IRC font" and select prefered font by using the cursor keys and finally confirming your choice by pressing Enter.<br>
<br>
Q: Changing IRC font does not change anything?<br>
A: Due to current design limitations of the IRC client the changes made won't take effect until next time the IRC client is started.<br>
<br>
Q: My settings got reset after rebooting!<br>
A: Make sure to save your settings by either using the Quick menu (System -> Save config) or by typing `savecfg` and pressing Enter in the terminal. If settings still fail to save it may be due to your system or emulator lacking SRAM, thus being unable to access the filesystem in SMDT.<br>
<br>
Q: How do I change my IRC nick?<br>
A: In the terminal, run the command `setvar username yournick` OR in the IRC client, type `/nick yournick` and press Enter.<br>
<br>
Q: How do I join a channel?<br>
A: Type `/join #channel` in the text input box at the bottom and press Enter.<br>
<br>
Q: I get double characters typed when I type on certain telnet servers?<br>
A: Either the server or SMDT failed to setup local echo. You can change this manually in the Quick meu (Settings -> Client settings -> Terminal -> Variables -> Local echo -> On).<br>