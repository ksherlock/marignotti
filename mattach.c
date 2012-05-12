#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <sys/socket.h>

#include "s16debug.h"

#pragma noroot
#pragma optimize 79

// better known as socket(2)
int mattach(int type, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word t;
    Word ipid;
    Entry *e;
    
    // p2 = selwakeup.
    selwakeupfx fx = (selwakeupfx)p2;
    int protocol = *(int *)p3;
    
    if (Debug > 0)
    {
        s16_debug_printf("socket type = %d protocol = %d",
            type, protocol);
    }
    
    switch (type)
    {
    case SOCK_STREAM:
    case SOCK_DGRAM:
        break;
    default: 
        return ESOCKTNOSUPPORT;
        break;
    }
    //if (protocol != 6) return EPROTONOSUPPORT;
    // TODO -- check protocol? 6 = tcp, 1 = icmp, 17 = udp.

    IncBusy();
    ipid = TCPIPLogin(MyID, 0, 0, 0, 0x0040);
    t = _toolErr;
    DecBusy();
    
    if (t) return ENETDOWN;
    
    e = create_entry(ipid);
    if (!e)
    {
        TCPIPLogout(ipid);
        return ENOMEM;
    }
    e->select_fx = fx;
    e->_TYPE = type;
    
    *(Word *)p1 = ipid;
    
    return 0;
}
