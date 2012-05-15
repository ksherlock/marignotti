#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include "net.h"
#include "s16debug.h"

#pragma noroot
#pragma optimize 79

int block(int sem)
{
    int xerrno = 0;
    Kswait(sem, &xerrno);

    return xerrno;
}


void copy_addr(xsockaddr_in *src, xsockaddr_in *dest, int *addrlen)
{
    int len;
    
    // bytswap the port.
    asm {
        ldy #2
        lda [src],y
        xba
        sta [src],y
    }

    len = 8;
    if (*addrlen < 8)
        len = *addrlen;
    else
        *addrlen = 8;
    
    // data is truncated if there isn't enough space.
    asm {
        ldx <len
        beq done
        ldy #0
        sep #0x20
    loop:
        lda [src],y
        sta [dest],y
        iny
        dex
        bne loop
        rep #0x20
    done:
    }
}

int queue_command(Entry *e, Word command, LongWord cookie, LongWord timeout)
{
    int xerrno;
    
    SEI();
    e->command = command;
    e->cookie = cookie;
    e->timeout = timeout;
    CLI();
    
    xerrno = 0;
    Kswait(e->semaphore, &xerrno);
    return xerrno;
} 




#pragma databank 1

int driver(
    int socknum, int req, 
    void *p1, void *p2,
    void *p3, void *p4, void *p5)
{
    int rv;
    Entry *e;
    Word oldBusy;
    
    s16_debug_printf("driver: %04x : %04x", socknum, req);

    
    oldBusy = *BusyFlag;
    
    if (req == PRU_ATTACH)
    {
        return mattach(socknum, p1, p2, p3, p4, p5);
    }

    
    e = find_entry(socknum);
    if (!e)
    {
        if (req == PRU_RCVD || req == PRU_SEND)
            *(LongWord *)p2 = 0;
            
        return EBADF;
    }

    switch (req)
    {
    case PRU_ABORT:
        break;
        
    case PRU_ACCEPT:
        return maccept(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_ATTACH:
        // KERNsocket(int domain, int type, int protocol, int *ERRNO);
        // handled above.
        break;
        
    case PRU_BIND:
        // KERNbind(int fd, struct sockaddr *my_addr, int addrlen, int *ERRNO)
        return mbind(socknum, p1, p2, p3, p4, p5);
        break;
        
    case PRU_CONNECT:
        // KERNconnect(int fd, struct sockaddr *serv_addr, 
        // int addrlen, int *ERRNO)
        return mconnect(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_CONNECT2:
        break;
        
    case PRU_CONTROL:
        return mioctl(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_DETACH:
        // called from GS/OS
        // int SOCKclose(int sock) part 2
        DecBusy();
        rv =  mdetach(e, p1, p2, p3, p4, p5);
        IncBusy();
        return rv;
        break;
        
    case PRU_DISCONNECT:
        // called from GS/OS
        // int SOCKclose(int sock) part 1
        //return do_disconnect(socknum, m, m_len, addr, addrlen, rights);
        return 0;
        break;
        
    case PRU_LISTEN:
        return mlisten(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_PEERADDR:
        return mgetpeername(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_RCVD:
        // this may be called from GSOS (in which case IncBusy()
        // is in effect
        // or from KERNrecvfrom (in which case it isn't).
        // 
        // may block, so be nice.
        // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
        if (oldBusy) DecBusy();
        rv = mread(e, p1, p2, p3, p4, p5);
        if (oldBusy) IncBusy();
        return rv;
        break;
        
    case PRU_RCVOOB:
        return mreadoob(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_SEND:
        // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
        // same as above.
        if (oldBusy) DecBusy();
        rv = mwrite(e, p1, p2, p3, p4, p5);
        if (oldBusy) IncBusy();
        return rv;
        break;
        
    case PRU_SENDOOB:
        // OOB is always inline.  so there.
        // this is never called via ReadGS.
        return mwrite(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_SENSE:
        break;
        
    case PRU_SHUTDOWN:
        return mshutdown(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_SOCKADDR:
        return mgetsockname(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_CO_GETOPT:
        return mgetsockopt(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_CO_SETOPT:
        return msetsockopt(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_SELECT:
        // 	int SOCKselect(int pid, int fl, int sock)
        return mselect(e, p1, p2, p3, p4, p5);
        break;
    }
    
    return EINVAL;
}

#pragma databank 0
