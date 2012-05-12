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

static int set_flag(void *p, int size, int flag)
{
    if (size == 4)
    {
        *(long *)p = flag;
        return 0;
    }
    if (size == 2)
    {
        *(int *)p = flag;
        return 0;
    }

    return EINVAL;
}

static int set_flag_long(void *p, int size, long flag)
{
    if (size == 4)
    {
        *(long *)p = flag;
        return 0;
    }
    if (size == 2)
    {
        *(int *)p = flag;
        return 0;
    }

    return EINVAL;
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
    
    
    // todo -- should set optlen = returned size.
    
    // todo -- linger
    switch (optname)
    {
    
    case SO_TYPE:
        // todo... non-stream 
        return set_flag(optval, optlen, e->_TYPE);
        break; 
        
    case SO_DEBUG:
        return set_flag(optval, optlen, e->_DEBUG);  
        break;

    case SO_REUSEADDR:
        return set_flag(optval, optlen, e->_REUSEADDR);  
        break;

    case SO_REUSEPORT:
        return set_flag(optval, optlen, e->_REUSEPORT);  
        break;

    case SO_KEEPALIVE:
        return set_flag(optval, optlen, e->_KEEPALIVE);  
        break;

    case SO_OOBINLINE:
        return set_flag(optval, optlen, 1);
        break;        
        
    case SO_SNDLOWAT:
        return set_flag(optval, optlen, e->_SNDLOWAT);
        break;
        
    case SO_RCVLOWAT:
        return set_flag(optval, optlen, e->_RCVLOWAT);
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
        if (t) terr = t;
        e->terr = terr;
        DecBusy();
        
        return set_flag_long(optval, optlen, e->sr.srRcvQueued);
        break;
    
    #endif
    
    #ifdef SO_NWRITE
    case SO_NWRITE:
        IncBusy();
        terr = TCPIPStatusTCP(e->ipid, &e->sr);
        t = _toolErr;
        if (t) terr = t;
        e->terr = terr;        
        DecBusy();

        return set_flag_long(optval, optlen, e->sr.srSndQueued);
        break;  
    #endif
    
    }

    return EINVAL;

}
