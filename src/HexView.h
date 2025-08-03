#ifndef HEXVIEW_H_INCLUDED
#define HEXVIEW_H_INCLUDED

#include <genesis.h>
#include "system/File.h"

void HexView_Input();
u16 HexView_Open(const char *filename);
void HexView_Close();

#endif // HEXVIEW_H_INCLUDED
