#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <misctool.h>
#include "s16debug.h"

#pragma noroot
#pragma optimize 79

int mconnect(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word port;
    Word t;
    Word terr;
    int xerrno;
    LongWord timeout;
    
    // todo -- if non-blocking, 
    // return EINPROGRESS 
    //
    
    xsockaddr_in *sin = (xsockaddr_in *)p3;
    int addrlen = *(int *)p4;
    
    port = sin->sin_port; 
    asm {
        lda <port
        xba
        sta <port
    }

    // check if already connected.
    IncBusy();
    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    t = _toolErr;
    if (t) terr = t;
    DecBusy();
    
    // todo -- if non-blocking, 
    // return EINPROGRESS first time, 
    // return EALREADY on subsequent calls.
    
    if (e->sr.srState != TCPSCLOSED)
        return EISCONN;
    

    IncBusy();
    TCPIPSetNewDestination(e->ipid, sin->sin_addr, port);
    t = _toolErr;
    DecBusy();
    
    if (t)
    {
        return ENETDOWN;
    }
    
    IncBusy();
    terr = TCPIPOpenTCP(e->ipid);
    t = _toolErr;
    if (t) terr = t;
    DecBusy();
    
    // todo -- better errors.
    if (terr)
    {
        return ENETDOWN;
    }


    timeout = GetTick() + 60 * 30;
    
    for (;;)
    {
    
        int xerrno;
        int state;
        
        xerrno = queue_command(e, kCommandConnect, 0, timeout);
        
        s16_debug_printf("mconnect: %d - %d - %d", 
          e->semaphore, xerrno, e->command);
        
        // hmmm .. should these abort?
        if (xerrno == EINTR)
        {
            IncBusy();
            e->command = kCommandNone;
            TCPIPAbortTCP(e->ipid);
            DecBusy();
            
            return EINTR; // ?
        }
        if (xerrno) return EIO; // semaphore destroyed?

        if (e->command) continue;  // reset to 0 if processed.
        
        state = e->sr.srState;
        if (state == TCPSESTABLISHED)
            return 0;
        
        if (state == TCPSCLOSED)
            // todo -- differentiate ECONNREFUSED vs EHOSTUNREACH
            return ECONNREFUSED;
        
        if (timeout && timeout <= GetTick())
        {
            IncBusy();
            TCPIPAbortTCP(e->ipid);
            DecBusy();
            
            return ETIMEDOUT;  
        }
    }
    
    
    return 0;  // should never hit.
    
}
