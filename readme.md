
# --- SMD Telnet/IRC/Terminal client v0.27+ ---
A telnet, IRC and terminal client for the Sega Mega Drive/Genesis with support for PS/2 keyboards and RS-232 communication.
![Screenshot of the telnet client](https://deceptsoft.com/smdtc_extra_git/telnet.png)
![Screenshot of the IRC client](https://deceptsoft.com/smdtc_extra_git/irc.png)
![Screenshot of the terminal emulator showing nano](https://deceptsoft.com/smdtc_extra_git/terminal_nano.png)
![Screenshot of a debugging utility to inspect streams](https://deceptsoft.com/smdtc_extra_git/hexview.png)

Extra hardware: 
1. PS/2 keyboard (not required but preferred).
2. A "voltage translator" to translate between the MD +5v and remote RS232 device logic levels,
I personally recommend the max3232 (https://www.ti.com/lit/ds/symlink/max3232.pdf)


## --- Disclaimer - READ ME OR FRY YOUR SYSTEM ---
> [!CAUTION]
**Do note that the MD is a 5 volt system!** That means you should take care to not connect any random serial device directly to the MD (Such as a PC).
Use a "voltage translator" such as the max3232 between your MD and remote device to translate the voltage levels.
Make sure you understand my ramble down under the "Device" section to hook up external devices correctly!
I (smds) will not take any responsibilities for any failure to read and understand the above warning.


## --- Credits ---
b1tsh1ft3r - Testing and improvement ideas
RKT - For creating a 4x8 extended ASCII font tileset
smds - Code


## --- Devices ---
SMDTC has a device manager which can detect certain devices to a degree. PS/2 devices such as a keyboard is one such device that can be detected.
Device detection is only done on bootup, no plug & play support (yet).

> [!IMPORTANT]
Devices are expected to be connected in a certain way to the MD controller ports, see the pin configuration list below.
PS/2 device pins (clock and data) are expected to be in a pair, for example:
If a keyboard use pin 1 for clock then pin 2 must be used for data.
If a keyboard use pin 3 for clock then pin 4 must be used for data.
Do not swap clock and data pins.

A total of 2 PS/2 devices and 1 UART device can be connected to a single MD controller port.

When a keyboard is connected and detected a 'K' icon will be visible in the status bar.
A fallback joypad device will be activated if SMDTC fails to find a keyboard or when a keyboard is plugged into PORT 2, allowing the use of a regular joypad.
However a keyboard is recommended to be able to actually type or use special functions.

> [!NOTE]
SMDTC cannot detect the presence of a serial connection (UART), by default SMDTC will listen on PORT 2. This setting can be changed in the quick menu -> mega drive settings -> select serial port.
Do not forget to save your changes by going to the quick menu -> reset -> save config to sram.

All detected devices can be viewed by going to the quick menu -> mega drive settings -> connected devices.

### Pin configuration of the MD controller ports: 
MD port pin 1 = PS/2 device 1 clock  (CLK1)
MD port pin 2 = PS/2 device 1 data   (DATA1)
MD port pin 3 = PS/2 device 2 clock  (CLK2)
MD port pin 4 = PS/2 device 2 data   (DATA2)
MD port pin 5 = VCC (+5V)
MD port pin 6 = Serial TX
MD port pin 7 = Reserved             (CP3)
MD port pin 8 = GND
MD port pin 9 = Serial RX

### Example keyboard wiring:
MD port pin 1 = Keyboard clock pin
MD port pin 2 = Keyboard data pin
MD port pin 5 = Keyboard VCC
MD port pin 8 = Keyboard GND

### MD UART pin reference:
MD port pin 5 = VCC
MD port pin 6 = TX
MD port pin 8 = GND
MD port pin 9 = RX

### Connected device list:
P1:0 = Port 1 @ pin 1+2
P1:1 = Port 1 @ pin 3+4
P1:S = Port 1 UART
P2:0 = Port 2 @ pin 1+2
P2:1 = Port 2 @ pin 3+4
P2:S = Port 2 UART
P3:0 = Port 3 @ pin 1+2
P3:1 = Port 3 @ pin 3+4
P3:S = Port 3 UART


## --- PS/2 Keyboard shortcuts ---
Right windows key = Open the Quick menu
Enter = Enter submenu and activate a choice
Escape = Back out of current menu
Up cursor = Move selector up
Down cursor = Move selector down
