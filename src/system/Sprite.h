#ifndef SPRITE_H_INCLUDED
#define SPRITE_H_INCLUDED

#include <genesis.h>

// Macro for setting up sprite attributes. n = sprite number between 0 and 79, v = value
#define GetSpriteAVR(n)              (AVR_SAT + ((n)*8))
#define SetSprite_Y(n, v)           *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(GetSpriteAVR(n) + 0); *((vu16*) VDP_DATA_PORT) = (v);
#define SetSprite_SIZELINK(n, s, l) *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(GetSpriteAVR(n) + 2); *((vu16*) VDP_DATA_PORT) = (((s) << 8) | (l)); 
#define SetSprite_TILE(n, v)        *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(GetSpriteAVR(n) + 4); *((vu16*) VDP_DATA_PORT) = (v);
#define SetSprite_X(n, v)           *((vu32*) VDP_CTRL_PORT) = VDP_WRITE_VRAM_ADDR(GetSpriteAVR(n) + 6); *((vu16*) VDP_DATA_PORT) = (v);

#define SPR_WIDTH_4x1  12
#define SPR_WIDTH_3x1  8
#define SPR_WIDTH_2x1  4
#define SPR_HEIGHT_1x4 3
#define SPR_HEIGHT_1x3 2
#define SPR_HEIGHT_1x2 1
#define SPR_SIZE_1x1   0

// Sprite indices
#define SPRITE_CURSOR  0     // Cursor sprite index
#define SPRITE_SCRSAV  1     // Screensaver sprite index
#define SPRITE_POINTER 2     // Mouse pointer
#define SPRITE_IRC     4     // IRC Text box

void SP_Setup();

#endif // SPRITE_H_INCLUDED
