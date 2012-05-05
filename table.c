#include "marignotti.h"

#include <string.h>
#include <misctool.h>

#pragma optimize 79
#pragma noroot

#define TABLE_SIZE 16
#define TABLE_MASK 15
static struct Entry *table[TABLE_SIZE];

void init_table(void)
{
    memset(table, 0, sizeof(table));
}

void destroy_table(void)
{

    Entry *e;
    unsigned i;
    
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        SEI();        
        e = table[i];
        table[i] = 0;
        CLI();
        
        while (e)
        {
            Entry *next;
            
            IncBusy();
            
            next = e->next;
            
            TCPIPAbortTCP(e->ipid);
            TCPIPLogout(e->ipid);
            
            sdelete(e->semaphore);
            free(e);
            
            e = next;
            
            DecBusy();
        }
        
    }
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
    
    tick = GetTick();
    
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        prev = NULL;
        e = table[i];
        while (e)
        {
            next = e->next;
        
            if (e->command)
            {
                Word expired = 0;
                Word sig = 0;
                Word state;
             
                IncBusy();

                
                if (e->timeout && tick > e->timeout)
                    expired = 1;
                
                terr = TCPIPStatusTCP(e->ipid, &e->sr);
                t = _toolErr;
                if (t) terr = t;
                e->terr = terr;
                
                state = e->sr.srState;
                
                switch(e->command)
                {
                case kCommandRead:
                    if (e->sr.srRcvQueued >= e->cookie
                        || expired
                        || terr)
                    {
                        sig = 1;
                    }
                    break;
                    
                case kCommandConnect:
                    if (state >= TCPSESTABLISHED || state == TCPSCLOSED)
                    {
                        sig = 1;
                    }
                    break;
                    
                case kCommandDisconnect:
                    if (state == TCPSCLOSED)
                    {
                        sig = 1;
                    }
                    break;
                
                case kCommandDisconnectAndLogout:
                    // logout and remove entry.
                    if (state == TCPSCLOSED)
                    {
                        TCPIPLogout(e->ipid);
                        sdelete(e->semaphore);
                        free(e);
                        e = NULL;
                        if (prev)
                        {
                            prev->next = next;
                        }
                        else
                        {
                            table[i] = next;
                        }
                    }
                    break;                    

                }

                if (sig)
                {
                    e->command = kCommandNone;
                    ssignal(e->semaphore);
                }

                DecBusy();
            } // e->command
                        
            e = next;
        }
    
    
    }

}