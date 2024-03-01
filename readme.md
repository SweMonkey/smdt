
# SMD Telnet/IRC/Terminal client v0.27+
A telnet, IRC and terminal client for the Sega Mega Drive/Genesis with support for PS/2 keyboards and RS-232 communication.<br>
![Screenshot of the telnet client](https://deceptsoft.com/smdtc_extra_git/telnet_small.png)
![Screenshot of the IRC client](https://deceptsoft.com/smdtc_extra_git/irc_small.png)
![Screenshot of the terminal emulator showing nano](https://deceptsoft.com/smdtc_extra_git/terminal_nano_small.png)
![Screenshot of a debugging utility to inspect streams](https://deceptsoft.com/smdtc_extra_git/hexview_small.png)

Extra hardware:<br>
1. PS/2 keyboard (not required but preferred).<br>
2. A "voltage translator" to translate between the MD +5v and remote RS232 device logic levels,<br>
I personally recommend the max3232 (https://www.ti.com/lit/ds/symlink/max3232.pdf)<br>


## Disclaimer - READ ME OR FRY YOUR SYSTEM
> [!CAUTION]
**Do note that the MD is a 5 volt system!** That means you should take care to not connect any random serial device directly to the MD (Such as a PC).<br>
Use a "voltage translator" such as the max3232 between your MD and remote device to translate the voltage levels.<br>
Make sure you understand my ramble down under the "Device" section to hook up external devices correctly!<br>
I (smds) will not take any responsibilities for any failure to read and understand the above warning.<br>


## Credits
b1tsh1ft3r - Testing and improvement ideas<br>
RKT - For creating a 4x8 extended ASCII font tileset<br>
smds - Code<br>


## Devices
SMDTC has a device manager which can detect certain devices to a degree. PS/2 devices such as a keyboard is one such device that can be detected.<br>
Device detection is only done on bootup, no plug & play support (yet).<br>

> [!IMPORTANT]
Devices are expected to be connected in a certain way to the MD controller ports, see the pin configuration list below.<br>
PS/2 device pins (clock and data) are expected to be in a pair, for example:<br>
If a keyboard use pin 1 for clock then pin 2 must be used for data.<br>
If a keyboard use pin 3 for clock then pin 4 must be used for data.<br>
Do not swap clock and data pins.<br>

A total of 2 PS/2 devices and 1 UART device can be connected to a single MD controller port.<br>
<br>
When a keyboard is connected and detected a 'K' icon will be visible in the status bar.<br>
A fallback joypad device will be activated if SMDTC fails to find a keyboard or when a keyboard is plugged into PORT 2, allowing the use of a regular joypad.<br>
However a keyboard is recommended to be able to actually type or use special functions.<br>

> [!NOTE]
SMDTC cannot detect the presence of a serial connection (UART), by default SMDTC will listen on PORT 2. This setting can be changed in the quick menu -> mega drive settings -> select serial port.<br>
Do not forget to save your changes by going to the quick menu -> reset -> save config to sram.<br>

All detected devices can be viewed by going to the quick menu -> mega drive settings -> connected devices.<br>

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

### Example keyboard wiring:
MD port pin 1 = Keyboard clock pin<br>
MD port pin 2 = Keyboard data pin<br>
MD port pin 5 = Keyboard VCC<br>
MD port pin 8 = Keyboard GND<br>

### MD UART pin reference:
MD port pin 5 = VCC<br>
MD port pin 6 = TX<br>
MD port pin 8 = GND<br>
MD port pin 9 = RX<br>

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


## PS/2 Keyboard shortcuts
Right windows key = Open the Quick menu<br>
Enter = Enter submenu and activate a choice<br>
Escape = Back out of current menu<br>
Up cursor = Move selector up<br>
Down cursor = Move selector down<br>
