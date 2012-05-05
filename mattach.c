#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <sys/socket.h>


#pragma noroot
#pragma optimize 79

// better known as socket(2)
int mattach(int type, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word t;
    Word ipid;
    Entry *e;
    
    // p2 = selwakeup.
    int protocol = *(int *)p3;
    
    if (type != SOCK_STREAM) return ESOCKTNOSUPPORT;
    if (protocol != 6) return EPROTONOSUPPORT;
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
    *(Word *)p1 = ipid;
    
    return 0;
}
