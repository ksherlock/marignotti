#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#include "s16debug.h"


#pragma noroot
#pragma optimize 79

int mgetpeername(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    destRec dr;
    xsockaddr_in tmp;

    xsockaddr_in *addr = (xsockaddr_in *)p3;
    int *addrlen = (int *)p4;

    if (Debug > 0)
    {
        s16_debug_printf("getpeername");
    }
    
    if (!addrlen) return EINVAL;
    if (!sock_addr) return EINVAL;
    
    IncBusy();
    TCPIPGetDestination(e->ipid, &dr);
    DecBusy();

    tmp.sin_family = AF_INET;
    tmp.sin_port = dr.drDestPort;
    tmp.sin_addr = dr.drDestIP;
    
    copy_addr(&tmp, addr, addrlen);

    return 0;
}