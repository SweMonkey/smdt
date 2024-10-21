#ifndef VARLIST_H_INCLUDED
#define VARLIST_H_INCLUDED

#include <genesis.h>

#define ST_BYTE 1
#define ST_WORD 2
#define ST_LONG 3
#define ST_SPTR 4
#define ST_SARR 5

typedef const struct s_varlist
{
    u8 size;
    void *ptr;
    const char *name;
} SM_VarList;

extern SM_VarList VarList[];

void getenv(const char *name, char *ret);

#endif // VARLIST_H_INCLUDED
