#include "marignotti.h"

#include <memory.h>
#include <texttool.h>
#include <misctool.h>
#include <locator.h>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <gno/gno.h>

#pragma optimize 79

int semID = 0;
Word MyID;
Word QuitFlag;

#pragma databank 0
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

#if 1
  // require 3.0b3
  if (TCPIPLongVersion() < 0x03006003)
  {
    if (fx) fx("Marinetti 3.0b3 is required.");
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

  if (flags == -1) exit(1);

  semID = screate(1);
    
  InstallNetDriver(driver, 0);

  QuitFlag = 0;  
  
  signal(SIGQUIT, signal_handler);
  signal(SIGINT, signal_handler);  
  signal(SIGHUP, signal_handler);  
  
  // SIGUSR to dump table ala netstat?
  while (!QuitFlag)
  {

    IncBusy();
        
    TCPIPPoll();   
    
    DecBusy(); 
    
    process_table();
    
    asm { cop 0x7f }
  }
  
  InstallNetDriver(NULL, 0);
  sdelete(semID);
  
  destroy_table();
  
  ShutDown(flags, 0, DisplayMessage);
  
  return 0;
} 
