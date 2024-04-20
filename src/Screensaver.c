#include "Screensaver.h"
#include "Utils.h"
#include "StateCtrl.h"

bool bScreensaver = FALSE;
u16 InactiveCounter = 0;
u16 InactiveUpper = 18000;  // NTSC = 18000 - PAL = 15000
static s16 ScrCx = 0;
static s16 ScrCy = 0;
static s16 ScrDx = 1;
static s16 ScrDy = 1;

// Cursor sprite tiles @ 0x10


void ScreensaverInit()
{
    InactiveCounter = 0;
    if (bPALSystem) InactiveUpper = 15000;   // NTSC = 18000 - PAL = 15000

    State cs = getState();

    if ((cs == PS_Telnet) || (cs == PS_Terminal))
    {
        // Set up screensaver sprite here - maybe beyond IRC sprites?
    }
}

void ScreensaverTick()
{
    if (bScreensaver == FALSE) return;

    if (InactiveCounter >= InactiveUpper)
    {
        // Reset cursor sprite (sprite 0) link to 0 when deactivating screensaver... somewhere

        //kprintf("Screensaver active");
        
        if ((ScrCy < 0) || (ScrCy > (bPALSystem?232:216)))
        ScrDy *= -1;

        if ((ScrCx < 0) || (ScrCx > 312))
        ScrDx *= -1;

        ScrCy += ScrDy;
        ScrCx += ScrDx;

        SetSprite_Y(0, ScrCy+128);
        SetSprite_X(0, ScrCx+128);
    }
    else 
    {
        InactiveCounter++;

        if (InactiveCounter == InactiveUpper)
        {
            // Update cursor sprite (sprite 0) link to screensaver sprite on screensaver activation here
        }
    }
}
