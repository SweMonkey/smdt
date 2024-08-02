#ifndef STDOUT_H_INCLUDED
#define STDOUT_H_INCLUDED

#include <genesis.h>
#include "Buffer.h"

extern Buffer stdout;
extern bool bAutoFlushStdout;

void Stdout_Push(const char *str);
void Stdout_PushByte(u8 byte);
void Stdout_Flush();

#endif // STDOUT_H_INCLUDED
