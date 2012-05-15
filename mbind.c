#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#include "s16debug.h"


#pragma noroot
#pragma optimize 79

int mbind(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word port;
    Word t;
    Word terr;

    xsockaddr_in *addr = (xsockaddr_in *)p3;
    int addrlen = p4 ? *(int *)p4 : 0;
    
    port = addr->sin_port; 
    asm {
        lda <port
        xba
        sta <port
    }
    
    if (Debug > 0)
    {
        union xsplit s;
        s.i32 = addr->sin_addr;
        s16_debug_printf("bind family = %d address = %d.%d.%d.%d port = %d",
            addr->sin_family,
            s.i8[0], s.i8[1], s.i8[2], s.i8[3], 
            port);
    }
    
    
    if (e->_TYPE != SOCK_STREAM)
        return EOPNOTSUPP;
    
    if (addrlen < 8 || !addr) return EINVAL;
    if (addr->sin_family != AF_INET) return EINVAL;
    
    // don't re-bind if listen() or connect() called.
    
    IncBusy();
    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    if (t) terr = t;
    e->terr = t;
    DecBusy();
    
    if (e->sr.srState != TCPSCLOSED) return EINVAL;
    
    // address component is ignored -- only port is used.

    IncBusy();
    TCPIPSetSourcePort(e->ipid, port);
    DecBusy();
    
    return 0;

}