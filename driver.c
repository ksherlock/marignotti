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
    
    s16_debug_printf("driver: %04x : %04x", socknum, req);

    
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
        break;
        
    case PRU_ATTACH:
        // KERNsocket(int domain, int type, int protocol, int *ERRNO);
        // handled above.
        break;
        
    case PRU_BIND:
        // KERNbind(int fd, struct sockaddr *my_addr, int addrlen, int *ERRNO)
        //return do_bind(socknum, m, m_len, addr, addrlen, rights);
        return 0;
        break;
        
    case PRU_CONNECT:
        // KERNconnect(int fd, struct sockaddr *serv_addr, 
        // int addrlen, int *ERRNO)
        return mconnect(e, p1, p2, p3, p4, p5);
        break;
        
    case PRU_CONNECT2:
        break;
        
    case PRU_CONTROL:
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
        break;
        
    case PRU_PEERADDR:
        break;
        
    case PRU_RCVD:
        // this may be called from GSOS (in which case IncBusy()
        // is in effect
        // or from KERNrecvfrom (in which case it isn't).
        // 
        // may block, so be nice.
        // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
        DecBusy();
        rv = mread(e, p1, p2, p3, p4, p5);
        IncBusy();
        return rv;
        break;
        
    case PRU_RCVOOB:
        break;
        
    case PRU_SEND:
        // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
        // same as above.
        DecBusy();
        rv = mwrite(e, p1, p2, p3, p4, p5);
        IncBusy();
        return rv;
        break;
        
    case PRU_SENDOOB:
        break;
        
    case PRU_SENSE:
        break;
        
    case PRU_SHUTDOWN:
        break;
        
    case PRU_SOCKADDR:
        break;
        
    case PRU_CO_GETOPT:
        break;
        
    case PRU_CO_SETOPT:
        break;
        
    case PRU_SELECT:
        // 	int SOCKselect(int pid, int fl, int sock)
        break;
    }
    
    return EINVAL;
}

#pragma databank 0
