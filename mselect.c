#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#pragma noroot
#pragma optimize 79

/*
 * return value is ignored.
 * 
 */

static boolean readable(Entry *e)
{
    if (e->_SHUT_RD)
        return false;
        
    if (e->sr.srState > TCPSESTABLISHED && e->sr.srRcvQueued)
        return true;

    if (e->sr.srRcvQueued >= e->_RCVLOWAT)
        return true;
        
    return false;
}

static boolean writable(Entry *e)
{
    if (e->_SHUT_WR)
        return false;
    
    if (e->sr.srState != TCPSESTABLISHED)
        return false;
    
    if (e->sr.srRcvQueued <= e->_SNDLOWAT)
        return true;
        
    return false;
}

static boolean exceptable(Entry *e)
{

    switch (e->terr)
    {
    case tcperrOK:
        break;
    case tcperrConClosing:
    case tcperrClosing:
    case tcperrConReset:
        if (!e->sr.srRcvQueued)
            return true;
        break;
    }

    if (e->sr.srRcvQueued)
        return false;
        
    if (e->sr.srNetworkError)
        return true;

    if (e->sr.srState > TCPSESTABLISHED)
        return true;
    
    return false;
}

int mselect(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word t;
    Word terr;
    
    int pid = *(int *)p2;
    int flag = *(int *)p3;

    *(int *)p3 = 0;

    IncBusy();
    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    t = _toolErr;
    if (t) terr = t;
    e->terr = terr;
    DecBusy();
        
    if (e->sr.srState == TCPSLISTEN)
    {
        // for a listen socket, "read" means "accept".
        switch (flag)
        {
        case 0:
            if (e->sr.srAcceptCount)
                *(int *)p3 = 1;
            break;
        }
    
    }
    else
    {
        switch (flag)
        {
        case 0:
            // readable.
            *(int *)p3 = readable(e);
            break;
            
        case 1:
            // writable.
            *(int *)p3 = writable(e);
            break;
            
        case 2:
            // exception.
            *(int *)p3 = exceptable(e);
            break;                

        }
    
    }
    
    
    return 0;
}