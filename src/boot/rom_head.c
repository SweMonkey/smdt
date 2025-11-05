#include "genesis.h"

__attribute__((externally_visible))
const ROMHeader rom_header = 
{
    "SEGA MEGA DRIVE ",                                 // System type
    "(C)DECEPTSOFT'25",                                 // Copyright/Release date
    "SMDT                                            ", // Domestic title
    "SMDT                                            ", // Overseas title
    "GM DSTC2017-33",                                   // Serial number
    0x000,                                              // Checksum
    "JKRDM           ",                                 // Device support
    0x00000000,                                         // ROM start address
    0x000FFFFF,                                         // ROM end address
    0xE0FF0000,                                         // RAM start address
    0xE0FFFFFF,                                         // RAM end address
    "RA",                                               // SRAM enabled
    0xF820,                                             // SRAM 8 bit ODD
    0x00200001,                                         // SRAM start address
    0x0021FFFF,                                         // SRAM end address
    "            ",                                     // Modem support
    "SMDT                                    ",         // Notes
    "EUJ             "                                  // Region support
};
