#include <tcpip.h>
#include <gno/gno.h>


enum {
    kCommandNone = 0,
    kCommandConnect,
    kCommandDisconnect,
    kCommandDisconnectAndLogout,
    kCommandRead,
    kCommandWrite,
    kCommandAbortAndLogout
};

typedef void (*selwakeupfx)(int collision, int pid);

typedef struct Entry {
    struct Entry *next;
    
    Word ipid;
    int semaphore;
    
    srBuff sr;
    Word terr;
    
    Word command;
    LongWord cookie;
    LongWord timeout;

    Word socket_type;
    
    // select magic.
    selwakeupfx select_fx;
    int select_rd_pid;
    int select_wr_pid;
    
    unsigned select_rd_collision:1;
    unsigned select_wr_collision:1;

    // shutdown(2)    
    unsigned _SHUT_RD:1;
    unsigned _SHUT_WR:1;

    // fcntl(2)
    unsigned _NONBLOCK:1;
    
    //setsockopt(2)
    unsigned _DEBUG:1;
    unsigned _REUSEADDR:1;
    unsigned _REUSEPORT:1;
    unsigned _KEEPALIVE:1;

    unsigned _OOBINLINE:1;
    unsigned _LINGER:1;
    unsigned _NOSIGPIPE:1;
    
    LongWord _SNDLOWAT;
    LongWord _RCVLOWAT;
    LongWord _RCVTIMEO;
    LongWord _SNDTIMEO;
    Word _LINGER_SEC;
    

} Entry;

extern Word MyID;
extern Word Debug;

typedef struct xsockaddr_in {
  short sin_family;
  unsigned short sin_port;
  unsigned long sin_addr;
  char sin_zero[8];
} xsockaddr_in;


#define IncBusy() asm { jsl 0xE10064 }
#define DecBusy() asm { jsl 0xE10068 }
#define Resched() asm { cop 0x7f }

#define BusyFlag ((byte *)0xE100FFl)

#define SEI() asm { sei }
#define CLI() asm { cli }

int block(int sem);
int queue_command(Entry *e, Word command, LongWord cookie, LongWord timeout);

void init_table(void);
void destroy_table(void);
void process_table(void);

Entry *find_entry(Word ipid);
Entry *create_entry(Word ipid);



// driver stuff.

int driver(int, int,  void *, void *, void *, void *, void *);

int mattach(int ipid, void *p1, void *p2, void *p3, void *p4, void *p5);

int mread(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mwrite(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mconnect(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mbind(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mdetach(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mdisconnect(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);

int mshutdown(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);


int mgetsockopt(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int msetsockopt(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);

int mioctl(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mgetsockname(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);
int mselect(Entry *, void *p1, void *p2, void *p3, void *p4, void *p5);



   
