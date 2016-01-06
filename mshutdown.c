#include "marignotti.h"
#include <gno/kerntool.h>
#include <errno.h>

#pragma noroot
#pragma optimize 79


int mshutdown(Entry *e, void *p1, void *p2, void *p3, void *p4, void *p5)
{
    int how = *(int *)p1;
    
    switch(how)
    {
    case 0: // shutrd
        e->_SHUT_RD = 1;
        break;
    case 1:
        e->_SHUT_WR = 1;
        break;
    case 2:
        e->_SHUT_RD = 1;
        e->_SHUT_WR = 1;
        break;
    default:
        return EINVAL;
    }
    return 0;
}
