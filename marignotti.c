/*
 * MariGNOtti
 *
 */
 
#pragma optimize 79

#include <memory.h>
#include <tcpip.h>
#include <texttool.h>
#include <misctool.h>
#include <locator.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

//#include <gno/kerntool.h>
#include <gno/gno.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

//#include <netinet/in.h>

#include "net.h"

volatile int QuitFlag;
static Word MyID;
int semID;


// this will be used to hold extra info (sockopts, etc)
// as well as to keep a list of ipids waiting to close.

typedef struct ipidHash {
    Word ipid;
    Word flags;
    struct ipidHash *next;
} ipidHash;

ipidHash *htable[64] = { };

ipidHash *dlist = NULL;



// not yet sure what this is for...
typedef void (*selwakeup)(int col_flag, int pid);

void debug_str(const char *cp)
{

    static Word Sweet16 = -1;
    
    
    if (Sweet16 == -1)
    {
        //volatile char *ereg = (char *)0x00c04f;
        /*
        // Apple II tech note 201
        */
        word emu_id = 0;
        word emu_version = 0;
        asm {
            // the lda ..,x is to prevent
            // the 2nd read from being optimized out.
            //brk 0xea
            lda #0
            ldx #0
            sep #0x20
            sta >0x00c04f,x
            lda >0x00c04f,x
            sta <emu_id
            lda >0x00c04f,x
            sta <emu_version
            rep #0x20
        }
        
        if (emu_id == 0x16 && emu_version >= 0x23) Sweet16 = 1;
        else Sweet16 = 0;
        
        //*EMUBYTE = 0;
        //if (*EMUBYTE == 0x16) Sweet16 = 1;
        //else Sweet16 = 0;
        #undef EMUBYTE
    }
    
    if (Sweet16 == 0) return;

    asm {
        ldx <cp+2
        ldy <cp
        cop 0x84
    }
}

void dump_bytes(const char *bytes, Word length)
{
    static const char *HexMap = "0123456789abcdef";
    static char buffer[80];
    
    while (length)
    {
        Word i, j, l;
        l = 16;
        if (l > length) l = length;
    
        memset(buffer, ' ', sizeof(buffer));
        
        for (i = 0, j = 0; i < l; ++i)
        {
            unsigned x = bytes[i];
            buffer[j++] = HexMap[x >> 4];
            buffer[j++] = HexMap[x & 0x0f];
            j++;
            if (i == 7) j++;
            
            buffer[50 + i] = isascii(x) && isprint(x) ? x : '.';
        }
        
        buffer[50 + 16 + 1] = 0;
        debug_str(buffer);
    
        length -= l;
        bytes += l;
    }

}

void dump_srbuff(const srBuff *sb)
{
static char buffer[128];

    sprintf(buffer, "%04x %04x %08lx %08lx %08lx %04x %04x %04x",
        sb->srState,
        sb->srNetworkError,
        sb->srSndQueued,
        sb->srRcvQueued,
        sb->srDestIP,
        sb->srDestPort,
        sb->srConnectType, 
        sb->srAcceptCount
    );

    debug_str(buffer);

}

int do_attach(
    int socknum, 
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights
)
{
    // returns errno or 0.
    Word ipid;
    ipidHash *e;
    
    int type = socknum;
    int *sockptr = (int *)m;
    selwakeup *fx = (selwakeup *)m_len;
    int *protocol = (int *)addr;
    
    //WriteLine("\pdo_attach");
    debug_str("do_attach");
    
    if (type != SOCK_STREAM) return EPROTONOSUPPORT;
    
    ipid = TCPIPLogin(MyID, 0, 0, 0, 0x0040);
    if (_toolErr) return EPFNOSUPPORT;
    
    // semaphore ...
    swait(semID);
    
    e = (ipidHash *)malloc(sizeof(ipidHash));
    e->ipid = ipid;
    e->flags = 0;
    e->next = htable[ipid & (64-1)];
    htable[ipid & (64 - 1)] = e;
    
    ssignal(semID);
    // end semaphore
    
    *sockptr = ipid;
    
    return 0;
}


// netinet/in.h is giving me compile errors...

struct xsockaddr_in {
  short sin_family;
  unsigned short sin_port;
  unsigned long sin_addr;
  char sin_zero[8];
};

int do_connect(
    int socknum, 
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    // returns errno or 0.
    Word terr;
    Longword tick, qtick;
    Word port;
    
    
    Word ipid = (Word)socknum;
    struct xsockaddr_in *sin = (struct xsockaddr_in *)addr;

    //WriteLine("\pdo_connect");
    debug_str("do_connect");
    
    // port is byte-swapped.
    
    port = sin->sin_port; 
    asm {
        lda <port
        xba
        sta <port
    }
    
    TCPIPSetNewDestination(ipid, 
        sin->sin_addr, 
        port);
        
    if (_toolErr) return EPFNOSUPPORT;
    
    terr = TCPIPOpenTCP(ipid);
    if (_toolErr || terr)
    {
        return EPFNOSUPPORT;
    }
    
    // wait to loop until actually connected.
    
    // 30-second timeout.
    qtick = GetTick() + 60 * 30;
    for(;;)
    {
        srBuff sr;
        
        terr = TCPIPStatusTCP(ipid, &sr);
        
        dump_srbuff(&sr);
        
        if (sr.srState == TCPSESTABLISHED)
            return 0;
        
        if (sr.srState == TCPSCLOSED)
            return ECONNREFUSED;
            
        if (sr.srState > TCPSESTABLISHED) 
            return ECONNREFUSED;
            
        if (GetTick() >= qtick)
            return ETIMEDOUT;
        
        // is this safe?
        sleep(1);
    }
    
    
    return 0;
}


int do_disconnect(
    int socknum,  
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    Word ipid = socknum;
    
    debug_str("do_disconnect");
    //WriteLine("\pdo_disconnect");

    // return value ignored.
    
    return 0;
}

int do_detach(
    int socknum,  
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    Word ipid = socknum;
    
    Word terr;
    ipidHash *e;
    ipidHash *prev;
    
    // returns a GS/OS error.
    
    // remove from hashtable.
    // begin disconnect sequence.
    // add to dlist.
    
    // need to use semaphore to block other thread...
    
    //WriteLine("\pdo_detach");
    debug_str("do_detach");
    
    debug_str("TCPIPCloseTCP");
    terr = TCPIPCloseTCP(ipid);
    
    // semaphore...
    debug_str("swait");
    swait(semID);
    
    prev = NULL;
    e = htable[ipid & (64-1)];
    while (e)
    {
        if (e->ipid == ipid) break;
        prev = e;
        e = e->next;
    }
    if (e)
    {
        if (prev) prev->next = e->next;
        else htable[ipid & (64 - 1)] = e->next;
        
        e->next = dlist;
        dlist = e;
    }
    
    debug_str("ssignal");
    ssignal(semID);
    // end semaphore.
    
    debug_str("return 0");
    return 0;
}

int do_send(
    int socknum, 
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    // called from GS/OS, busyflag set 
    // (seems like a bug to me....)
    
    Word terr;
    Word ipid = (Word)socknum;
    void *data = (void *)m;
    size_t *len = (size_t *)m_len;
    
    //WriteLine("\pdo_send");
    
    {
        static char buffer[64];
        sprintf(buffer, "do_send: ipid = %04x len = %04x",
            ipid, (Word)*len
        );
        debug_str(buffer);
    }
    
    terr = TCPIPWriteTCP(ipid,  data, *len, 0, 0);
    
    // closing == eof.
    if (terr == tcperrConClosing)
    {
        *len = 0;
        return ECONNRESET;
    }
    
    return 0;
}


int do_rcvd(
    int socknum, 
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    // called from GS/OS, busy flag is set.
    
    // todo -- EWOULDBLOCK?
    
    Word terr;
    Word ipid = (Word)socknum;
    void *data = (void *)m;
    size_t *len = (size_t *)m_len;
    
    rrBuff rr;
    
    //WriteLine("\pdo_rcvd");

    {
        static char buffer[64];
        sprintf(buffer, "do_rcvd: ipid = %04x len = %04x",
            ipid, (Word)*len
        );
        debug_str(buffer);
    }

    // todo -- should block unless O_NONBLOCK set.
        
    for (;;)
    {
    
        srBuff sr;
        
        terr = TCPIPStatusTCP(ipid, &sr);
        
        dump_srbuff(&sr);
        
            
        terr = TCPIPReadTCP(ipid,  0, (Ref)data, *len, &rr);
    
        if (rr.rrBuffCount)
        {
            {
                static char buffer[80];
                
                sprintf(buffer, "read %04x bytes %04x %04x %04x",
                  (Word)rr.rrBuffCount,
                  rr.rrMoreFlag,
                  rr.rrPushFlag,
                  rr.rrUrgentFlag
                  );
                debug_str(buffer);
                
                dump_bytes(data, rr.rrBuffCount);
            }
            
            *len = rr.rrBuffCount;
            return 0;
        }
        
        if (terr == tcperrBadConnection)
            return ENOTCONN;
     
        // no data, no more data, then closing.       
        if (terr == tcperrConClosing && !rr.rrMoreFlag)
            return ECONNRESET;
        
        
        // if NON_BLOCKING, return EWOULDBLOCK.
        
        //asm {
        //    cop 0x7f
        //}
        // poll in main loop not called, apparently.
        
        //TCPIPPoll(); // 
        sleep(1); // cop 0x7f?
    }
    
    return 0;
}

#pragma databank 1
static int driver(
    int socknum, int req, 
    void *m, size_t *m_len,
    struct sockaddr *addr, int *addrlen, 
    void *rights)
{
    int rv;

    {
        static char buffer[64];
        sprintf(buffer, "driver: req = %d", req);
        debug_str(buffer);
    }

    switch (req)
    {
    case PRU_ABORT:
        break;
        
    case PRU_ACCEPT:
        break;
        
    case PRU_ATTACH:
        // KERNsocket(int domain, int type, int protocol, int *ERRNO);
        return do_attach(socknum, m, m_len, addr, addrlen, rights);
        break;
        
    case PRU_BIND:
        //WriteLine("\pbind!");
        // todo
        return 0;
        break;
        
    case PRU_CONNECT:
        // KERNconnect(int fd, struct sockaddr *serv_addr, 
        // int addrlen, int *ERRNO)
        return do_connect(socknum, m, m_len, addr, addrlen, rights);
        break;
        
    case PRU_CONNECT2:
        break;
        
    case PRU_CONTROL:
        break;
        
    case PRU_DETACH:
        // called from GS/OS
        // int SOCKclose(int sock) part 2
        return do_detach(socknum, m, m_len, addr, addrlen, rights);
        break;
        
    case PRU_DISCONNECT:
        // called from GS/OS
        // int SOCKclose(int sock) part 1
        return do_disconnect(socknum, m, m_len, addr, addrlen, rights);
        break;
        
    case PRU_LISTEN:
        break;
        
    case PRU_PEERADDR:
        break;
        
    case PRU_RCVD:
        // called from GS/OS.
        // may block, so be nice.
        // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
        asm {
            // dec busy.
            jsl 0xE10068
        }
        rv = do_rcvd(socknum, m, m_len, addr, addrlen, rights);
        asm {
            // inc busy.
            jsl 0xE10064
        }
        return rv;
        break;
        
    case PRU_RCVOOB:
        break;
        
    case PRU_SEND:
            // SOCKrdwr(struct rwPBlock *pb, word cmd, int sock)
            return do_send(socknum, m, m_len, addr, addrlen, rights);
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
        break;
    }
    
    return EINVAL;
}


void signal_handler(int sig, int code)
{
    WriteLine("\pWe got signal!");
    QuitFlag = 1;
}

void DisplayMessage(const char *string)
{
    if (string) WriteLine(string);
}

#pragma databank 0

// startup/shutdown flags.
enum {
  kLoaded = 1,
  kStarted = 2,
  kConnected = 4
};

Word StartUp(displayPtr fx)
{
  word status;
  word flags = 0;
  
  // TCPIP is an init, not a tool, so it should always
  // be loaded.
  
  status = TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(54, 0x0300);
    if (_toolErr) return -1;

    status = 0;
    flags |= kLoaded;
  }

#if 0
  // require 3.0b3
  if (TCPIPLongVersion() < 0x03006003)
  {
    if (flags & kLoaded)
      UnloadOneTool(54);
    return -1;      
  }
#endif

  if (!status)
  {
    TCPIPStartUp();
    if (_toolErr) return -1;
    flags |= kStarted;
  }

  status = TCPIPGetConnectStatus();
  if (!status)
  {
    TCPIPConnect(fx);
    flags |= kConnected;
  }

  return flags;
}

void ShutDown(word flags, Boolean force, displayPtr fx)
{
    if (flags & kConnected)
    {
      TCPIPDisconnect(force, fx);
      if (_toolErr) return;
    }
    if (flags & kStarted)
    {
      TCPIPShutDown();
      if (_toolErr) return;
    }
    if (flags & kLoaded)
    {
      UnloadOneTool(54);
    }
}


int main(int argc, char **argv)
{

  int flags;

  MyID = MMStartUp();

  flags = StartUp(DisplayMessage);

  semID = screate(1);
    
  InstallNetDriver(driver, 0);

  QuitFlag = 0;  
  
  signal(SIGQUIT, signal_handler);
  signal(SIGINT, signal_handler);  
  signal(SIGHUP, signal_handler);  
  
  while(!QuitFlag)
  {
    ipidHash *e;
    ipidHash *prev;
    
    debug_str("TCPIPPoll() begin");
    TCPIPPoll();
    debug_str("TCPIPPoll() end");
    
    // monitor disconnects...
    // semaphore ...
    
    swait(semID);
    if (errno == EINTR) continue;
    
    e = dlist;
    prev = NULL;
    
    while (e)
    {
        Word terr;
        srBuff sr;
        ipidHash *next;

        next = e->next;
                
        terr = TCPIPStatusTCP(e->ipid, &sr);
        
        dump_srbuff(&sr);
        
        // timed wait or closed ...
        if (sr.srState == TCPSCLOSED)
        {
            
            TCPIPLogout(e->ipid);
            free(e);
    
            if (prev)
            {
                prev->next = next;
            }
            else
            {
                dlist = next;
            }
        }    
        else
        {
            prev = e;
        }
        
        e = next;
    }
    
    ssignal(semID);
    // end semaphore.
    
    sleep(1); // need a sleep10 to match alarm10
  }
  
  InstallNetDriver(NULL, 0);
  sdelete(semID);

  
  ShutDown(flags, 0, DisplayMessage);
  
  return 0;
} 
