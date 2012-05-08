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

static boolean get_flag(void *p, int size, Word *flag)
{
    if (size == 4)
    {
        *flag = *(LongWord *)flag;
        return true;
    }
    if (size == 2)
    {
        *flag = *(Word *)flag;
        return true;
    }
    if (size == 1)
    {
        *flag = *(Byte *)flag;
        return true;
    }
    
    return false;
}

static boolean get_flag_long(void *p, int size, LongWord *flag)
{
    if (size == 4)
    {
        *flag = *(LongWord *)flag;
        return true;
    }
    if (size == 2)
    {
        *flag = *(Word *)flag;
        return true;
    }
    if (size == 1)
    {
        *flag = *(Byte *)flag;
        return true;
    }
    
    return false;
}

int msetsockopt(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word terr;
    Word t;
    Word flag;
    
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
    
    case SO_DEBUG:
        if (!get_flag(optval, optlen, &flag))
            return EINVAL;
        e->_DEBUG = flag ? 1 : 0;
        return 0;
    
    case SO_REUSEADDR:
        if (!get_flag(optval, optlen, &flag))
            return EINVAL;
        e->_REUSEADDR = flag ? 1 : 0;
        return 0;
        
    case SO_REUSEPORT:
        if (!get_flag(optval, optlen, &flag))
            return EINVAL;
        e->_REUSEPORT = flag ? 1 : 0;
        return 0;
        
    case SO_KEEPALIVE:
        if (!get_flag(optval, optlen, &flag))
            return EINVAL;
        e->_KEEPALIVE = flag ? 1 : 0;
        return 0;
    
    case SO_OOBINLINE:
        // always 1.
        if (!get_flag(optval, optlen, &flag))
            return EINVAL;
        if (!flag)
            return EINVAL;
        return 0;
        break;

            
    case SO_SNDLOWAT:
        // 0 is valid.
        if (!get_flag_long(optval, optlen, &e->_SNDLOWAT)) 
            return EINVAL;
        return 0;
        break;
    
    
    case SO_RCVLOWAT:
        // min size = 1.
        if (!get_flag_long(optval, optlen, &e->_RCVLOWAT)) 
            return EINVAL;
        if (e->_RCVLOWAT) return 0;
        
        e->_RCVLOWAT = 1;
        return EINVAL;
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