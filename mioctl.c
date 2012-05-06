#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#include <sys/ioctl.h>

#pragma noroot
#pragma optimize 79

int mioctl(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word t;
    Word terr;
    
    LongWord tioc = *(LongWord *)p1;
    void *data = (void *)p3;
    
    if (tioc == FIONREAD)
    {
        // return # of bytes available 
        IncBusy();
        terr = TCPIPStatusTCP(e->ipid, &e->sr);
        t = _toolErr;
        DecBusy();
        if (t) terr = t;
        
        if (data) *(Word *)data = e->sr.srRcvQueued;
        
        return 0;
    }
    
    if (tioc == FIONBIO)
    {
        // set/clear non-blocking io.
        int tmp;
        
        tmp = data ? *(int *)data : 0;
        e->_NONBLOCK = tmp ? 1 : 0;
        return 0;
    }
    
    // SIOCSHIWAT / SIOCGHIWAT ?
    // SIOCSLOWAT / SIOCGLOWAT ?
    
    return EINVAL;
}