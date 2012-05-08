#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <string.h>

#include <sys/socket.h>

#pragma noroot
#pragma optimize 79

int mgetsockname(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    LongWord addr;
    Word port;
    int len;
    
    xsockaddr_in *sock_addr = (xsockaddr_in *)p3;
    int *addrlen = (int *)p4;
    
    if (!addrlen) return EINVAL;
    if (!sock_addr) return EINVAL;

    len = *addrlen;

    // bsd { has char sin_len; char sin_family; ... }
    // gno has { short sin_family; ... }

    
    IncBusy();
    port = TCPIPGetSourcePort(e->ipid);
    addr = TCPIPGetMyIPAddress();
    DecBusy();
    
    memset(sock_addr, 0, len);
    
    // truncate missing data.
    if (len >= 2)
        sock_addr->sin_family = AF_INET;
    
    if (len >= 4)
    {
        asm {
            lda <port;
            xba
            sta <port;
        }
        sock_addr->sin_port = port;
    }
    
    if (len >= 8)
    {
        sock_addr->sin_addr = addr;
    }
    
    
    return 0;
}