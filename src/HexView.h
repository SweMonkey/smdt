#ifndef HEXVIEW_H_INCLUDED
#define HEXVIEW_H_INCLUDED

#include <genesis.h>
#include "system/File.h"

extern bool bShowHexView;

void HexViewFile_Input();
void HexView_Open(const char *filename);
void HexView_Close();

#endif // HEXVIEW_H_INCLUDED
