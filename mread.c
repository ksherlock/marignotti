#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#include <misctool.h>

#pragma noroot
#pragma optimize 79

// called through GSOS.
int mread(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    rrBuff rr;

    int xerrno = 0;
    LongWord count;
    LongWord timeout;
    Word terr;
    Word t;
    
    char *buffer = (char *)p1;
    LongWord nbytes = *(LongWord *)p2;
    *(LongWord *)p2 = 0;

    
    count = e->_RCVLOWAT;
    if (count > nbytes) count = nbytes;

    // call immediately if possible, otherwise queue it up.
    IncBusy();
    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    t = _toolErr;
    DecBusy();
    if (t) terr = t;
    
    if (t || terr == tcperrBadConnection || terr == tcperrNoResources)
    {
        return ENOTCONN;
    }
    
    // eof or data available?
    
    if (e->sr.srRcvQueued >= count || terr == tcperrConClosing)
    {        
        count = e->sr.srRcvQueued;
        if (count > nbytes) count = nbytes;
        
        if (count)
        {
            IncBusy();
            terr = TCPIPReadTCP(e->ipid, 0, (Ref)buffer, count, &rr);
            t = _toolErr;
            DecBusy();
            if (t) terr = t;

            *(LongWord *)p2 = rr.rrBuffCount;    
        }
        return 0;
    }
        
    if (e->_NONBLOCK) return EAGAIN;


    if (e->_RCVTIMEO) 
        timeout = GetTick() + e->_RCVTIMEO;   
    else
        timeout = 0;
        
    for(;;)
    {
        xerrno = queue_command(e, kCommandRead, count, timeout);
        if (xerrno == EINTR) return EINTR;
        if (xerrno) return EIO;
        
        if (e->command) continue;  // reset to 0 if processed.
         
        terr = e->terr;
        
        if (terr  && terr != tcperrConClosing)
        {
            return EIO;
        }
        
        // 3 things may have happened.
        // 1. sufficient data available.
        // 2. connection closed.
        // 3. timeout.

        
        if (e->sr.srRcvQueued >= count || terr == tcperrConClosing)
        {
            count = e->sr.srRcvQueued;
            if (count > nbytes) count = nbytes;
        
            if (count)
            {
                IncBusy();
                terr = TCPIPReadTCP(e->ipid, 0, (Ref)buffer, count, &rr);
                t = _toolErr;
                DecBusy();
                                
                *(LongWord *)p2 = rr.rrBuffCount;
            }
            return 0;
        }
                
        if (timeout && timeout > GetTick())
            return EAGAIN;
            
    }

    // should not hit...
    return 0;
}

