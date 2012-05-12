#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>
#include <sys/socket.h>

#include <memory.h>
#include <misctool.h>

#include "s16debug.h"

#pragma noroot
#pragma optimize 79

typedef struct xipheader {
    byte verlen;
    byte tos;
    word len;
    word id;
    word fragoff;
    byte ttl;
    byte proto;
    word cksum;
    longword src;
    longword dst;
    // verlen & 0x0f is the number of 32-but words
    // if > 5, extra data present and should be skipped.
    
    // data...
    
} xipheader;

typedef struct xudpheader {
    word source;
    word dst;
    word len;
    word cksum;
    // data...
} xudpheader;

// todo -- should have addr/addrlen for recvfrom.
static int sock_read(
    Entry *e, 
    void *buffer, 
    LongWord nbytes, 
    LongWord *outbytes)
{
    LongWord size;
    LongWord lowat;
    Word t;
    Word terr;
    rrBuff rr;

    
    if (e->_TYPE == SOCK_DGRAM)
    {
        // todo -- address support.
        
        // manually parse the ip header and udp header.
        // 
        
        
        Handle h;
        xipheader *ip;
        Word offset = 0;
        char *cp;
        

        h = TCPIPGetNextDatagram(e->ipid, protocolUDP, 0);
        t = _toolErr;
        
        if (t) return ENETDOWN;
        if (!h) return EAGAIN;
        
        size = GetHandleSize(h);
        
        HLock(h);
        cp = *(char **)h;
        
        ip = (xipheader *)cp;
        offset = (ip->verlen & 0x0f) << 2;
        
        // would get the address here.
        // ip->src
        
        offset += u_data;
        
        if (offset >= size)
        {
            // ???
            DisposeHandle(h);
            return 0;
        }
        
        size -= offset;
        cp += offset;
        
        // nbytes is the max read... any overflow is discarded.
        if (size > nbytes) size = nbytes;
        BlockMove(cp, buffer, nbytes);
                
        if (Debug > 1)
        {
            s16_debug_dump(buffer, (Word)size);
        }
        
        DisposeHandle(h);
        
        *outbytes = size;
        return 0;
    }
    
    // todo -- address support (which will always be the dest ip...)

    terr = TCPIPStatusTCP(e->ipid, &e->sr);
    t = _toolErr;
    if (t) terr = t;
    e->terr = terr;
    
    if (t) return ENETDOWN;
    
    if (e->sr.srState < TCPSESTABLISHED) return ENOTCONN;
    
    size = e->sr.srRcvQueued;
    lowat = e->_RCVLOWAT;
    
    // if the state is established, read LOWAT...nbytes.
    // if the connection is closing, LOWAT is 1.
    
    if (e->sr.srState > TCPSESTABLISHED)
    {
        lowat = 1;
        if (!size) return 0; // eof
    }
    
    if (size < nbytes && size >= lowat)
        nbytes = size;

    if (size < nbytes) return EAGAIN;
    
    // size >= nbytes.
    terr = TCPIPReadTCP(e->ipid, 0, (Ref)buffer, nbytes, &rr);
    t = _toolErr;
    if (t) terr = t;
    e->terr = terr;

    // todo -- if there was a push, < nbyte will have been read.
    // loop until nbytes have been read?

    if (Debug > 1)
    {
        s16_debug_dump(buffer, (Word)rr.rrBuffCount);
    }

    *outbytes = rr.rrBuffCount;
    return 0;

}

// called through ReadGS, recv, recvfrom
int mread(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{

    int xerrno = 0;
    LongWord timeout;

    
    char *buffer = (char *)p1;
    LongWord nbytes = *(LongWord *)p2;
    xsockaddr *addr = (xsockaddr *)p3;
    int addrlen = p4 ? *(int *)p4 : 0;
    
    LongWord *outbytes = (LongWord *)p2;
    
    *outbytes = 0;

    if (Debug > 0)
    {
        s16_debug_printf("read nbytes = %ld", nbytes);
    }

    IncBusy();
    xerrno = sock_read(e, buffer, nbytes, outbytes);
    DecBusy();
    
    if (xerrno != EAGAIN || e->_NONBLOCK)
    {
        return xerrno;
    }
    
    
    if (e->_RCVTIMEO) 
        timeout = GetTick() + e->_RCVTIMEO;   
    else
        timeout = 0;
        
    for(;;)
    {
        xerrno = queue_command(e, kCommandRead, nbytes, timeout);
        if (xerrno == EINTR) return EINTR;
        if (xerrno) return EIO;
        
        if (e->command) continue;  // reset to 0 if processed.
         

        IncBusy();
        xerrno = sock_read(e, buffer, nbytes, outbytes);
        DecBusy(); 
        
        if (xerrno != EAGAIN) return xerrno;
        
        if (timeout && timeout <= GetTick())
            return EAGAIN;
    }

    // should not hit...
    return 0;
}

