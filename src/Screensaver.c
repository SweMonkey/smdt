#include "Screensaver.h"
#include "Utils.h"
#include "Palette.h"
#include "StateCtrl.h"

bool sv_bScreensaver = TRUE;    // Screensaver enable
s16 InactiveCounter = 0;
s16 InactiveUpper = 18000;      // NTSC = 18000 - PAL = 15000
static s16 ScrCx = 0;
static s16 ScrCy = 0;
static s16 ScrDx = 1;
static s16 ScrDy = 1;


void ScreensaverInit()
{
    InactiveCounter = 0;
    if (bPALSystem) InactiveUpper = 15000;   // NTSC = 18000 - PAL = 15000

    // Set up screensaver sprite here
    SetSprite_Y(SPRITE_ID_SCRSAV, 0);
    SetSprite_X(SPRITE_ID_SCRSAV, 0);
    SetSprite_TILE(SPRITE_ID_SCRSAV, (0xE000 | AVR_SCRSAV));
}

void ScreensaverTick()
{
    if (sv_bScreensaver == FALSE) return;

    if (InactiveCounter >= InactiveUpper)   // Screensaver is active, tick the sprite movement
    {        
        if ((ScrCy < 0) || (ScrCy > (bPALSystem?208:192)))
        {
            ScrDy *= -1;
            SetColor(55, random());
        }

        if ((ScrCx < 0) || (ScrCx > 288))
        {
            ScrDx *= -1;
            SetColor(55, random());
        }

        ScrCy += ScrDy;
        ScrCx += ScrDx;

        SetSprite_Y(SPRITE_ID_SCRSAV, ScrCy+128);
        SetSprite_X(SPRITE_ID_SCRSAV, ScrCx+128);
    }
    else if (InactiveCounter <= -1) // Turn off screensaver
    {
        SetSprite_Y(SPRITE_ID_SCRSAV, 0);
        SetSprite_X(SPRITE_ID_SCRSAV, 0);

        InactiveCounter = 0;
    }
    else // Screensaver is inactive, tick the inactive counter instead
    {
        InactiveCounter++;
    }
}
