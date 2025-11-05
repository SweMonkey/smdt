#ifndef STDOUT_H_INCLUDED
#define STDOUT_H_INCLUDED

#include <genesis.h>
#include "Buffer.h"
#include "system/File.h"

extern Buffer StdoutBuffer;
extern Buffer StdinBuffer;

extern bool bAutoFlushStdout;

extern SM_File *tty_in;
extern SM_File *tty_out;
extern SM_File *stdout;
extern SM_File *stdin;
extern SM_File *stderr;

#define printf(...) F_Printf(stdout, __VA_ARGS__)

void IO_CreatePseudoFiles();
void IO_ForceRestorePseudoFiles();

void Stdout_Push(const char *str);
void Stdout_PushByte(u8 byte);
void Stdout_Flush();

#endif // STDOUT_H_INCLUDED
