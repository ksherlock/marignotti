#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#include <sys/socket.h>
#include <sys/time.h>

#pragma noroot
#pragma optimize 79

static LongWord timeval_to_ticks(struct timeval tv)
{
    LongWord rv;
    
    rv = 0;
    if (tv.tv_usec || tv.tv_sec)
    {
        
        if (tv.tv_sec)
            rv = 60 * tv.tv_sec;        
        
        // usec = 1/1,000,000
        // split into 60 
        if (tv.tv_usec)
            rv += ((tv.tv_usec + 16666) / 16667);
            
        if (!rv) rv = 1;
    }
    
    return rv;    
}


int msetsockopt(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word terr;
    Word t;
    
    int level = *(int *)p1;
    int optname = *(int *)p2;
    void *optval = (void *)p3;
    int optlen = p4 ? *(int *)p4 : 0;
    
    if (level != SOL_SOCKET) return EINVAL;
    if (!optval) return EINVAL;
    
    // todo -- linger.
    // todo -- oobinline (error if 0?)
    switch(optname)
    {
    
    case SO_OOBINLINE:
        if (optlen == 4)
        {
            Word flag = *(LongWord *)optval;
            if (!flag) return EINVAL;
            return 0;
        }
        if (optlen == 2)
        {
            Word flag = *(Word *)optval;
            if (!flag) return EINVAL;
            return 0;        
        }
        if (optlen == 1)
        {
            Word flag = *(char *)optval;
            if (!flag) return EINVAL;
            return 0;
        }
        break;
            
    case SO_SNDLOWAT:
        if (optlen == 4)
        {
             e->_SNDLOWAT = *(LongWord *)optval;
            return 0;
        }
        if (optlen == 2)
        {
             e->_SNDLOWAT = *(Word *)optval;
            return 0;        
        }
        break;
    
    
    case SO_RCVLOWAT:
        if (optlen == 4)
        {
             e->_RCVLOWAT = *(LongWord *)optval;
            return 0;
        }
        if (optlen == 2)
        {
             e->_RCVLOWAT = *(Word *)optval;
            return 0;        
        }
        break; 
        
    case SO_SNDTIMEO:
        if (optlen == sizeof(struct timeval))
        {
            // stored as ticks aka seconds * 60.
            struct timeval *tv = (struct timeval *)optval;
            
            e->_SNDTIMEO = timeval_to_ticks(*tv);
            return 0;
        }
        break;
    
    case SO_RCVTIMEO:
        if (optlen == sizeof(struct timeval))
        {
            // stored as ticks aka seconds * 60.
            struct timeval *tv = (struct timeval *)optval;
            
            e->_RCVTIMEO = timeval_to_ticks(*tv);
            return 0;
        }
        break;        
        
    }
    
    return EINVAL;   
}