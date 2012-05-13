#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <sys/socket.h>


#include "s16debug.h"


static int sock_accept(
    Entry *e, 
    int *newfd, 
    selwakeupfx fx, 
    xsockaddr_in *addr, 
    int *addrlen)
{
    int ipid;
    Word t;
    Entry *child;
    
    IncBusy();
    ipid = TCPIPAcceptTCP(e->ipid, 0);
    t = _toolErr;
    DecBusy();

    if (t == terrNOINCOMING) return EAGAIN;
    if (t == terrNOTSERVER) return EINVAL;
    if (t) return ENETDOWN; // ?

    child = create_entry(ipid);
    if (!child)
    {
        TCPIPAbortTCP(ipid);
        TCPIPLogout(ipid);
        return ENOMEM;
    }
    
    // set up child options.
    child->_TYPE = SOCK_STREAM;
    child->select_fx = fx;
    
    // address...
    if (addr && addrlen)
    {
        if (*addrlen >= 8)
        {
            destRec dr;
            Word port;

            IncBusy();
            TCPIPGetDestination(ipid, &dr);
            DecBusy();
            
            port = dr.drDestPort;
            
            asm {
                lda <port
                xba
                sta <port
            }            
            addr->sin_port = port;
            addr->sin_addr = dr.drDestIP;
            addr->sin_family = AF_INET;
            
            *addrlen = 8;
        }
        else
        {
            *addrlen = 0;
        }

    }
    
    *newfd = ipid;
    return 0;
}

int maccept(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    int t;
    Word ipid;
    int xerrno;
    
    int *newfd = (int *)p1;
    selwakeupfx fx = (selwakeupfx)p2;
    xsockaddr_in *addr = (xsockaddr_in *)p3;
    int *addrlen = (int *)p4;
    
    
    if (Debug > 0)
    {
        s16_debug_printf("accept");
    }
    
    
    if (e->_TYPE != SOCK_STREAM) return EOPNOTSUPP;


    xerrno = sock_accept(e, newfd, fx, addr, addrlen);
    
    if (xerrno != EAGAIN || e->_NONBLOCK)
    {
        return xerrno;
    }
    
    
    for (;;)
    {
        xerrno = queue_command(e, kCommandAccept, 0, 0);
        if (xerrno == EINTR) return EINTR;
        if (xerrno) return EIO;
        
        if (e->command) continue;  // reset to 0 if processed.

        xerrno = sock_accept(e, newfd, fx, addr, addrlen);

        if (xerrno != EAGAIN) return xerrno;        
    }


    return 0;
}