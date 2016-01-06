#include "marignotti.h"

#include <string.h>
#include <misctool.h>
#include "s16debug.h"


#pragma optimize 79
#pragma noroot


extern boolean readable(Entry *e);

#define TABLE_SIZE 16
#define TABLE_MASK 15
static struct Entry *table[TABLE_SIZE];

// ipids waiting to close + logout.
static struct Entry *dlist;

static void destroy_entry(Entry *e, Boolean abort);

void init_table(void)
{
    dlist = NULL;
    memset(table, 0, sizeof(table));
}

void destroy_table(void)
{

    Entry *e;
    Entry *next;
    unsigned i;
    
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        SEI();        
        e = table[i];
        table[i] = 0;
        CLI();
        
        while (e)
        {
            IncBusy();
            
            next = e->next;
            destroy_entry(e, true);
            e = next;
            
            DecBusy();
        }
    }
    
    e = dlist;
    while (e)
    {
        next = e->next;
        destroy_entry(e, true);
        e = next;
    }
}

static void destroy_entry(Entry *e, Boolean abort)
{
    if (abort) TCPIPAbortTCP(e->ipid);
    
    TCPIPLogout(e->ipid);
    sdelete(e->semaphore);
    free(e);    
}

Entry *find_entry(Word ipid)
{
    Entry *e;

    IncBusy();   
    e = table[ipid & TABLE_MASK];
    
    while (e)
    {
        if (e->ipid == ipid) break;
        e = e->next;
    }
    DecBusy();

    return e;
}

Entry *create_entry(Word ipid)
{
    Entry *e;
    e = NULL;
    IncBusy();
    e = calloc(sizeof(Entry), 1);
    DecBusy();
    
    if (!e) return NULL;
    e->semaphore = screate(0);
    if (e->semaphore < 0)
    {
        IncBusy();
        free(e);
        DecBusy();
        return NULL;
    }

    e->ipid = ipid;
    e->_OOBINLINE = 1;
    e->_SNDLOWAT = 1024;
    e->_RCVLOWAT = 1;
    
    e->select_rd_pid = 0xffff;
    e->select_wr_pid = 0xffff;
    
    SEI();
    e->next = table[ipid & TABLE_MASK];
    table[ipid & TABLE_MASK] = e;
    CLI();
    
    return e;
}

void process_table(void)
{
    Word terr;
    Word t;
    LongWord tick;
    
    unsigned i;
    Entry *e;
    Entry *next;
    Entry *prev;
    
    
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        prev = NULL;
        e = table[i];
        while (e)
        {
            Word command;
            next = e->next;
        
        
            // select.
            // do this first ... a close would invalidate.
            if (e->select_rd_pid != 0xffff)
            {
                if (readable(e))
                {
                    Word coll;
                    Word pid;
                    
                    SEI()
                    
                    coll = e->select_rd_collision;
                    pid = e->select_rd_pid;
                    
                    e->select_rd_pid = 0xffff;
                    e->select_rd_collision = 0;
                                        
                    CLI()
                    if (e->select_fx)
                        e->select_fx(coll, pid);
                    
                    if (Debug > 0)
                    {
                        s16_debug_printf("select/read wakeup.");
                    }
                    
                }
            
            }
            
                    
        
            command = e->command;
            if (command)
            {
                Word expired;
                Word sig;
                Word state;
             
                tick = GetTick();
                IncBusy();
                
                expired = 0;
                if (e->timeout != 0 && tick > e->timeout)
                    expired = 1;
                
                terr = TCPIPStatusTCP(e->ipid, &e->sr);
                t = _toolErr;
                if (t) terr = t;
                e->terr = terr;
                
                if (Debug > 2)
                {
                    s16_debug_printf("- ipid: %d command %d", 
                        e->ipid, command);
                  
                    s16_debug_srbuff(&e->sr);
                }
                state = e->sr.srState;
                sig = 0;
                
                switch(command)
                {
                case kCommandRead:
                    // block until data available.
                    if (expired
                        || e->sr.srRcvQueued >= e->cookie
                        || e->sr.srRcvQueued >= e->_RCVLOWAT
                        || terr)
                    {
                        sig = 1;
                    }
                    break;
                    
                
                case kCommandWrite:
                    // block until data sent.
                    if (e->sr.srSndQueued <= e->cookie
                        || expired
                        || terr)
                    {
                        sig = 1;
                    }
                    break;
                
                case kCommandAccept:
                    if (e->sr.srAcceptCount > 0)
                    {
                        sig = 1;
                    }
                    break;
                    
                case kCommandConnect:
                    // block until connection established.
                    if (state >= TCPSESTABLISHED || state == TCPSCLOSED)
                    {
                        sig = 1;
                    }
                    if (expired)
                    {
                        sig = 1;
                    }
                    break;
                    
                case kCommandDisconnect:
                    // block until connection closed.
                    if (state == TCPSCLOSED)
                    {
                        sig = 1;
                    }
                    break;
                
                case kCommandDisconnectAndLogout:
                
                    // move to other list
                    // since it's no longer a valid fd
                    
                    if (prev)
                    {
                        prev->next = next;
                    }
                    else
                    {
                        table[i] = next;
                    }
                    
                    // add to the dlist.
                    e->next = dlist;
                    dlist = e;
                    e = NULL;
                    
                    break;                    

                }

                if (sig)
                {
                    s16_debug_printf("sending signal to %d", e->semaphore);
                    e->command = kCommandNone;
                    ssignal(e->semaphore);
                }

                DecBusy();
            } // e->command
            
            

                 
            if (e) prev = e;   
            e = next;
        }
    
    
    }
    
    
    // now process the disconnect list.
    e = dlist;
    prev = NULL;
    while (e)
    {
        next = e->next;
        
        IncBusy();
        terr = TCPIPStatusTCP(e->ipid, &e->sr);
        t = _toolErr;
        if (t) terr = t;
        DecBusy();
        
        if (e->sr.srState == TCPSCLOSED)
        {
            destroy_entry(e, false);
            
            e = NULL;
            // remove..
            if (prev)
            {
                prev->next = next;
            }
            else
            {
                dlist = next;
            }            
        }
        
        if (e) prev = e;
        e = next;
    }

}
