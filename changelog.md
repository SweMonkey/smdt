##v0.29:
IRC: Cursor fix.<br>
Added initial support for xPico module connected to an MD UART port.<br>
PS2KB: Reworked keyboard reading method, it now works on NTSC/MD2 models (Thanks to b1tsh1ft3r).<br>
PS2KB: SendCommand() has been rewritten.<br>
Quality of life improvement to the terminal emulator.<br>
Terminal/Telnet: Code speed improvements/readability decreased<br>
Cursor stuff; 1. Now alternates tile instead of colour, visually the same but now it is actually transparent when it is in the "off" state. 2. Cursor colour can be set and saved.<br>
Terminal: Better BG colouring support when using custom BG colour.<br>
Terminal: More built in commands. See "help" command in SMDTC.<br>
Improved the screensaver. It now has its own 32x32 sprite.
IRC: Fixed some hardcoded addresses that should be using the common address macros.<br>
IRC: More dynamic allocations to free up more RAM outside of the IRC client. malloc / free was not used before because it was broken.<br>

##v0.28:
Added initial support for b1tsh1ft3r's network cartridge 'RetroLink', allowing much faster network speeds and a MD to be completely stand alone.<br>
Added a screensaver that will activate after 5 minutes of inactivity. Disabled by default.<br>
Buffer system can no longer drop a byte if it is full.<br>
SMDTC start menu has been revamped and simplified. It can now accept addresses to connect to if using retrolink.<br>
VRAM management; Hardcoded addresses all over the codebase has been replaced with common macros (defines). This makes it easy to move around various chunks in VRAM.<br>
Sprite macros; Make it easier to manipulate more than 1 hardcoded sprite.<br>
Logging macros changed around to make it easier to log specific parts.<br>
Old unused macros removed and lot of code clean ups (How much is now broken I wonder...).<br>
Fixed old bug with ESC[A / B / C / D; It didn't always move the cursor by at least 1. Margins only partially implemented.<br>
Terminal: Added "DEC Special Character and Line Drawing Set" character map.<br>
Terminal: Allow printing characters > $7E again; Fixes some issues with certain telnet servers using extended ascii.<br>
Terminal: The terminal state is now acting as a command interpreter, which can be used to type commands and launch sessions with arguments.<br>
Added support for the Sega Saturn keyboard.<br>
PS2KB: Sending commands to a PS/2 keyboard now works. Turns out I was sending the data bits in reverse order.<br>
Device manager cleanup. Keyboards are now initialized externally.<br>
Device manager can now check for sega device identifiers on connected devices.<br>
SMDTC can now handle multiple different keyboards (Not at the same time however).<br>
Initial doxygen comment additions.<br>
Telnet: Title strings should no longer keep appending onto itself when disconnecting and connecting to a server.<br>

##v0.27:
SRAM: Added the ability to save config to SRAM.<br>
IRC: Added "tabs" to have several channels/private messaging going at once. It uses 8-bit tilemap buffers in RAM, 
it is limited to 3 concurrent "tabs" at once due to RAM requirements (3*8KB = 24KB RAM used). Use F1/F2/F3 to change tab.<br>
IRC: Added a typing input at the bottom of the screen.<br>
IRC: Commands implemented, prefixed with '/'. Use '/raw <IRC COMMAND>' to send raw IRC command strings.<br>
IRC: You can now initiate a private conversation.<br>
IRC: User list added, press F5 to show/hide window.<br>
IRC: Can now change username via the /nick <your_nick> command.<br>
Telnet: Added a proper bell... ok PSG beeps.<br>
Terminal: Added the terminal emulator as its own launchable session type.<br>
TRM: Some windowing cleanup. (TRM = Window plane UI backend).<br>
NET: Moved Networking stuff out of the terminal emulator (Rx/Tx etc) into a new unit 'NET'.<br>
PS2KB: Added timeout code when in the middle of reading a bitstream. May cause dropped key input when the MD is very busy with network traffic. This blocking bug has existed at least since v0.20.<br>
PS2KB: Added Swedish keyboard layout.<br>
Terminal: Added colour support for 4x8 fonts.<br>
Terminal: You can now select to use normal or highlighted colours when using 4x8 fonts with colour. (TODO: Custom colours?).<br>
Terminal: Added a lot of new escape codes; window ops, 256 colour handling and a lot of other minor but obscure escape codes.<br>
Fixed some tables that was supposed to be in ROM but instead got placed in RAM (type const * const varname).<br>
Various bugfixes, most related to mIRC colour fuckery and 4x8 fonts (clearing and printing).<br>
Added some extra Rx buffer statistics.<br>
Cursor tile can now change shapes; block, underline, bar, invisible. Blinking can now also be toggled.<br>

##v0.26:
DevMgr: Device manager created. Devices can now be mapped to pin 1+2 or pin 3+4 on any port. 2 PS/2 devices can be on the same port as a UART device.<br>
DevMgr: Serial/UART device can be in any port now (not recommended to change port while tx/rx is ongoing).<br>
DevMgr: Added a temporary crude device detection system. Joypad will be enabled on port 1 only when no keyboard is detected or when keyboard is not in port 1.<br>
PS2KB: Initial HOST->PS/2 communication (PS/2 response is always "rx fail, please resend command" though).<br>
Quickmenu: Some updates regarding device list and selecting UART port.<br>
Quickmenu: Text colour is selectable when using 4x8 font.<br>
Telnet: Added some missing escape codes (skeleton code mostly).<br>
Terminal and telnet: A few tweaks related to scrolling, cursor handling and special characters.<br>
Terminal: Blinky cursor is back. It used to cause lockups, lets hope it doesn't anymore.<br>
More UTF8 "characters" added.<br>

##v0.25:
Telnet: More IAC functionality (LINEMODE, TERM_SPEED, SEND_LOCATION, ENVIROMENT_OP, SUPRESS_GO_AHEAD) and enabled the previously disabled echo settings.<br>
Telnet: LINEMODE EDIT setting now controls whether to send bytes in bulk (allowing for local editing) or byte-by-byte to remote server.<br>
Telnet: Fixes to IAC NAWS - Now takes into account column settings (based on font size for now instead of actual 40/80 column setting) and row settings (29 for PAL and 27 for NTSC -- 1 row is always used for status bar at the top).<br>
PC Side: smdt-pc to replace socat has been created, capable of piping and logging Tx/Rx streams.<br>
IRC: Initial IRC support (Line parser/command interpreter).<br>
IRC: mIRC colour support.<br>
IRC: CTCP Support (Version and ping only).<br>
Terminal: Testing 256 column mode for 4x8 fonts and added left/right scrolling for 4x8 fonts.<br>
Terminal: Rewritten MoveCursor function since it was old and buggy.<br>
Terminal: Emulator scrolling bugfixes.<br>
PS2KB: Moved out telnet state specific code.<br>
PS2KB: Rewritten scancode handler, now half the code size and supports all keys/combinations and keyboard layouts (US only for now) - Still TODO: CapsLock/NumLock/ScrollLock handling is not implemented.<br>
A million tiny little tweaks here and there.<br>

##v0.24:
Split terminal and telnet client apart (to prepare for other protocols).<br>
Implemented input subsystem.<br>
Changed KB/JP to new input subsystem.<br>
Implemented statemachine (to prepare for other protocols).<br>
Added a startup menu.<br>
Initial IRC state.<br>

##v0.23:
Lowered keyboard polling timeout, it caused the text to be printed out in chunks (not very smooth looking).<br>
Detecting (and ignoring) UTF BOM.<br>
Made status icons more attractive.<br>
Added sent/received bytes statistics.<br>
Added hex viewer for RX Buffer.<br>

##v0.22 - 2023-11-11:
Added a quick and dirty antialiased 4x8 font.<br>
Added a circular buffer for incoming bytes.<br>
Fixed cursor going rogue when switching between font sizes.<br>

##v0.21 - 2023-11-10:
Added support for 4x8 font (thanks to RKT).<br>

##v0.21 - 2023-11-09:
Added baud speed selector (300, 1200, 2400, 4800).<br>


#--- Older changes ---

##v0.1x-0.20 - 2023-xx-xx:
Implemented IAC code sequence handling.<br>
Implemented basic UTF-8 decoding.<br>
Rewritten escape code sequence handling.<br>
Rewritten line clearing and character printing functions.<br>
Implemented a menu system for changing terminal settings.<br>
Fixed shift key on keyboard not actually shifting letters.<br>
Added terminal type selector (XTerm, ANSI, VT100, MEGADRIVE and UNKNOWN).<br>

