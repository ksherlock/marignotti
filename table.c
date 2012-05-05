#include "marignotti.h"

#include <string.h>
#include <misctool.h>
#include "s16debug.h"


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
    
    
    for (i = 0; i < TABLE_SIZE; ++i)
    {
        prev = NULL;
        e = table[i];
        while (e)
        {
            Word command;
            next = e->next;
        
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
                
                s16_debug_printf("process: %04x : %04x : expired: %d", 
                  e->ipid, command, expired);
                  
                s16_debug_srbuff(&e->sr);
                
                state = e->sr.srState;
                sig = 0;
                
                switch(command)
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
                    if (expired)
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
                    if (expired)
                    {
                        // sweet 16 link layer?
                        TCPIPAbortTCP(e->ipid);
                        state = TCPSCLOSED;
                    }
                    
                    if (state == TCPSCLOSED || state == TCPSTIMEWAIT)
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
                    s16_debug_printf("sending signal to %d", e->semaphore);
                    e->command = kCommandNone;
                    ssignal(e->semaphore);
                }

                DecBusy();
            } // e->command
                        
            e = next;
        }
    
    
    }

}