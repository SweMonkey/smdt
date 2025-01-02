#include "genesis.h"

__attribute__((externally_visible))
const ROMHeader rom_header = 
{
    "SEGA MEGA DRIVE ",                                 // System type
    "(C)DECEPTSOFT'24",                                 // Copyright/Release date
    "SMD Terminal Emulator                           ", // Domestic title
    "SMD Terminal Emulator                           ", // Overseas title
    "GM DSTC2017-32",                                   // Serial number
    0x000,                                              // Checksum
    "JKRD            ",                                 // Device support
    0x00000000,                                         // ROM start address
    0x000FFFFF,                                         // ROM end address
    0xE0FF0000,                                         // RAM start address
    0xE0FFFFFF,                                         // RAM end address
    "RA",                                               // SRAM enabled
    0xF820,                                             // SRAM 8 bit ODD
    0x00200000,                                         // SRAM start address
    0x0021FFFF,                                         // SRAM end address
    "            ",                                     // Modem support
    "SMD Terminal Emulator                   ",         // Notes
    "EUJ             "                                  // Region support
};
