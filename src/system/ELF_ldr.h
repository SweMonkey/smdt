#ifndef ELF_LDR_H_INCLUDED
#define ELF_LDR_H_INCLUDED

#include "Utils.h"

#ifdef KERNEL_BUILD

void *ELF_LoadProc(const char *fn);

#endif // KERNEL_BUILD

#endif // ELF_LDR_H_INCLUDED
