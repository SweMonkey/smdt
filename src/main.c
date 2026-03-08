#include <genesis.h>
#include "StateCtrl.h"
#include "system/Init.h"

int main(bool hardReset)
{
    SystemInit(hardReset);

    while(TRUE)
    {
        StateTick();
        SYS_doVBlankProcess();
    }

    return 0;
}
