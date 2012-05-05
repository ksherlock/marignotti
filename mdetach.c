#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <misctool.h>

#pragma noroot
#pragma optimize 79

// 
int mdetach(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{

    // TODO -- SO_LINGER/SO_LINGER_SEC
    LongWord timeout;
    
    timeout = GetTick() + 5 * 60;
    SEI();
    e->command = kCommandDisconnectAndLogout;
    e->cookie = 0;
    e->timeout = timeout;
    CLI();
    
    return 0; 
}