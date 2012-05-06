#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/time.h>

#include <intmath.h>

#pragma noroot
#pragma optimize 79

static void ticks_to_timeval(LongWord ticks, struct timeval *tv)
{
    LongDivRec qr;
    
    if (ticks == 0)
    {
        tv->tv_sec = 0;
        tv->tv_usec = 0;
    }
    else
    {
        qr = LongDivide(ticks, 60);
    
        tv->tv_sec = qr.quotient;
        tv->tv_usec = qr.remainder * 16667;
        // qr.remainder * 1,000,000 / 60
    }
    
}

int mgetsockopt(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word terr;
    Word t;
    
    int level = *(int *)p1;
    int optname = *(int *)p2;
    void *optval = (void *)p3;
    int optlen = p4 ? *(int *)p4 : 0;
    
    if (level != SOL_SOCKET) return EINVAL;
    if (!optval) return EINVAL;
    
    
    // todo -- linger
    switch (optname)
    {
    
    case SO_TYPE:
        // todo... non-stream 
        if (optlen == 4)
        {
            *(LongWord *)optval = SOCK_STREAM;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = SOCK_STREAM;
            return 0;        
        }
        break; 
        

    case SO_OOBINLINE:
        if (optlen == 4)
        {
            *(LongWord *)optval = 1;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = 1;
            return 0;      
        }
        if (optlen == 1)
        {
            *(char *)optval = 1;
            return 0;
        }
        break;        
        
        
    case SO_SNDLOWAT:
        if (optlen == 4)
        {
            *(LongWord *)optval = e->_SNDLOWAT;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = e->_SNDLOWAT;
            return 0;        
        }
        break;
        
    case SO_RCVLOWAT:
        if (optlen == 4)
        {
            *(LongWord *)optval = e->_RCVLOWAT;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = e->_RCVLOWAT;
            return 0;        
        }
        break;
            
    case SO_SNDTIMEO:
        if (optlen == sizeof(struct timeval))
        {
            // stored as ticks aka seconds * 60.
            struct timeval *tv = (struct timeval *)optval;
            
            ticks_to_timeval(e->_SNDTIMEO, tv);
            return 0;
        }
        break;
    
    case SO_RCVTIMEO:
        if (optlen == sizeof(struct timeval))
        {
            // stored as ticks aka seconds * 60.
            struct timeval *tv = (struct timeval *)optval;
            
            ticks_to_timeval(e->_RCVTIMEO, tv);
            return 0;
        }
        break;
        
    #ifdef SO_NREAD
    case SO_NREAD:
        IncBusy();
        terr = TCPIPStatusTCP(e->ipid, &e->sr);
        t = _toolErr;
        DecBusy();
        if (t) terr = t;
        
        if (optlen == 4)
        {
            *(LongWord *)optval = e->sr.srRcvQueued;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = e->sr.srRcvQueued;
            return 0;
        }
        break;
    
    #endif
    
    #ifdef SO_NWRITE
    case SO_NWRITE:
        IncBusy();
        terr = TCPIPStatusTCP(e->ipid, &e->sr);
        t = _toolErr;
        DecBusy();
        if (t) terr = t;

        if (optlen == 4)
        {
            *(LongWord *)optval = e->sr.srSndQueued;
            return 0;
        }
        if (optlen == 2)
        {
            *(Word *)optval = e->sr.srSndQueued;
            return 0;
        }
        return EINVAL;
        break;  
    #endif
    
    }

    return EINVAL;

}
