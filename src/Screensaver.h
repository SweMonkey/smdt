#ifndef SCREENSAVER_H_INCLUDED
#define SCREENSAVER_H_INCLUDED

#include <genesis.h>

extern bool bScreensaver;
extern u16 InactiveCounter;

void ScreensaverInit();
void ScreensaverTick();

#endif // SCREENSAVER_H_INCLUDED
