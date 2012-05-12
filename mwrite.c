#include "marignotti.h"
#include <gno/kerntool.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <errno.h>

#include "s16debug.h"

#pragma noroot
#pragma optimize 79

// WriteGS, send, or sendto
int mwrite(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word terr;
    Word t;
    int xerrno;
    
    char *buffer = (char *)p1;
    LongWord nbytes = *(LongWord *)p2;
    xsockaddr *addr = (xsockaddr *)p3;
    int addrlen = p4 ? *(int *)p4 : 0;
    
    LongWord *outbytes = (LongWord *)p2;
    
    *outbytes = 0;
    
    if (Debug > 0)
    {
        s16_debug_printf("write nbytes = %ld", nbytes);
        if (addr)
        {
            s16_debug_puts(" to address...");
            s16_debug_dump(addr, addrlen);
        }
    }
    if (Debug > 1)
    {
        s16_debug_puts("");
        s16_debug_dump(buffer, (Word)nbytes);
    }
    
    if (e->_TYPE == SOCK_DGRAM)
    {
        // TCPIPSendUDP builds a header
        // using the source port, my ip address,
        // destination port and destination ip address.
        
        // mconnect should probably just set the destination
        // address/port and exit.
        
        // if an address was specified, we would need to 
        // build the udp header here.
        IncBusy();
        TCPIPSendUDP(e->ipid, buffer, nbytes);
        t = _toolErr;        
        DecBusy();
        
        if (t) return ENETDOWN;
        
        *outbytes = nbytes;
        return 0;
    }
    
    // todo -- queue up if pending >= _SNDLOWAT?
    // todo -- push?
    
    IncBusy();
    terr = TCPIPWriteTCP(e->ipid, buffer, nbytes, 0, 0);
    t = _toolErr;
    if (t) terr = t;
    e->terr = terr;
    DecBusy();
    
    if (t || terr == tcperrBadConnection)
        return ENOTCONN;

    if (terr == tcperrNoResources)
        return ENOMEM;
    
    if (terr == tcperrConClosing)
    {
        if (!e->_NOSIGPIPE)
        {
            int xerrno;
            Kkill(Kgetpid(), SIGPIPE, &xerrno);
        }
            
        return EPIPE;
    }
    
    *outbytes = nbytes;
        
    return 0;
}