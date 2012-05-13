#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <sys/socket.h>

#include "s16debug.h"

#pragma noroot
#pragma optimize 79

/*
 * return value is ignored.
 * 
 */
 
 
 /*
  * select is rather ugly...
  * listen and connect pass in a select wakeup function which
  * takes two parameters, a collision flag and the pid to awaken.
  * 
  * when select is called, it returns immediately, in the affirmative 
  * if possible.  Otherwise, it stores the pid and the collision flag. 
  * When the condition is met, the select wakeup function is called
  * and the pid and collision flag are cleared.
  *
  * exceptions (which really mean OOB) are not supported.
  * write currently always returns immediately, in the affirmative.
  * for a server, "read" means a connection is pending.
  * otherwise, it means the read will not block (ie, data is available
  * or the connection has closed.
  *
  * this is all based on looking at the select.asm and driver source
  * code, so it could be wrong...
  */
 

boolean readable(Entry *e)
{

    Word state;
    Word terr;
    Word t;
    
    state = e->sr.srState;

    // hmm.. read will return 0 immediately.
    if (e->_SHUT_RD)
        return true;
        
    if (e->_TYPE == SOCK_DGRAM)
    {
        Word count;
        /*
        udpVars uv;
        IncBusy();
        TCPIPStatusUDP(e->ipid, &uv);
        //t = _toolErr;
        DecBusy();
        
        return uv.uvQueueSize > 0;
        */
        IncBusy();
        count = TCPIPGetDatagramCount(e->ipid, protocolUDP);
        DecBusy();
        
        return count > 0;
    }
    

    IncBusy();
    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    t = _toolErr;
    if (t) terr = t;
    e->terr = terr;
    DecBusy();
    
    // for a server, "read" means "accept".
    if (state == TCPSLISTEN)
    {
        return e->sr.srAcceptCount > 0;
    }
    
    // eof means readable.
    if (state == TCPSCLOSED || state > TCPSESTABLISHED)
        return true;
    
    /*
     * The receive low-water mark is the amount of data that must be 
     * in the socket receive buffer for select to return "readable." 
     * It defaults to 1 for TCP, UDP, and SCTP sockets.
     *
     * - UNIX Network Programming Volume 1, Third Edition, page 169
     *
     */
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
    return 0;
    // never.
}

int mselect(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    Word t;
    Word terr;
    
    int pid = *(int *)p2;
    int flag = *(int *)p3;
    int *outflag = (int *)p3;
    
    *outflag = 0;


    if (Debug > 0)
    {
        s16_debug_printf("select pid = %d flag = %d", pid, flag);
    }

    switch (flag)
    {
    case 0:
        if (readable(e))
        {
            *outflag = 1;
            return 0;
        }
        break;
    case 1:
        //always writable. [?]
        *outflag = 1;
        return 0;
        break;
    case 2:
        // no exceptions.
        return 0;
        break;
    default:
        return 0;
        break;
    }
    
    
    // main loop will call sel flag when readable.

    SEI();
    
    if (e->select_rd_pid == 0xffff)
        e->select_rd_pid = pid;
    else if (e->select_rd_pid != pid)
        e->select_rd_collision = 1;
        
    CLI();
    
    return 0;
}