#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#include "s16debug.h"


#pragma noroot
#pragma optimize 79

int mlisten(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word port;
    Word terr;
    Word t;
    
    int backlog = p1 ? *(int *)p1 : 0;
    
    if (Debug > 0)
    {
        s16_debug_printf("listen backlog = %d", backlog);
    }
    
    
    if (e->_TYPE != SOCK_STREAM)
        return EOPNOTSUPP;
        
    IncBusy();    
    terr = TCPIPListenTCP(e->ipid);
    t = _toolErr;
    if (t) terr = t;
    DecBusy();
    
    if (t) return ENETDOWN;
    
    // not in TCPSCLOSED state.
    if (terr == tcperrConExists) return EINVAL;
    if (terr) return EINVAL; // other errors?
    
    return 0;
}