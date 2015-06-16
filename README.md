MariGNOtti
==========

MariGNOtti is a network driver for GNO/ME 2.0.6.  It provides a translation 
layer between BSD sockets and Marinetti. Think of it as a replacement for 
GSTCP.

Installation:
-------------

1. copy the `marignotti`, `ftp`, `sic`, and `whois` executables to your `/usr/local/bin` directory.
  * `ftp` is an update of the gno 2.0.6 ftp (it adds passive ftp support).
  * `whois` is an updated version (from FreeBSD 9.0).  The gno 2.0.6 version is from FreeBSD 2.1.
  * `sic` is a very simple irc client. 

    Example:
    
        sic -h irc.a2central.com -p 6667 -n myname
        (a bunch of messages)
        :j #a2c.chat << join a room

2. copy the `etc/services` file to `/etc/services` (if it does not exist).

3. edit and copy the `etc/resolve.conf` file to `/etc/resolve.conf` (if it does 
not exist). This file is your DNS server

4. If you don not have a DNS server, edit `/etc/hosts` to include your favorite 
sites.



Usage:
------

Run marignotti in the background (`marignotti &`).  To quit, bring it to the 
foreground (`fg`) and hit `control-C`.

If you are using the Sweet-16 emulator version 3.0, `marignotti -d` will print
some debugging information to the debugger console.  `marignotti -dd` will 
enable extra debugging information (including the contents of reads and 
writes).


What it does:
-------------

While marignotti is running in the background, it translates BSD socket calls 
into Marinetti tool calls.  It also handles blocking IO, socket options, 
select, etc.  


Architecture:
-------------

BSD sockets are, by default, blocking whereas everything Marinetti does 
(other than the inital network connection) is non-blocking.  To handle 
blocking behavior, each socket has a semaphore.  The main thread performs 
all the polling and uses the semaphore to signal the other threads when they 
can proceed.


Supported:
----------

`socket` -- only support for `PF_UNIX`, `SOCK_STREAM`, and `SOCK_DGRAM`.

`connect` -- but non-blocking connects are not yet supported.

`bind` -- currently a no-op since the old `whois` used it.

`read`/`recv`/`recvfrom` -- supports blocking, non-blocking, timeouts, and `RCVLOWAT`.  
The address parameter of `recvfrom` is not supported.  OOB is not supported.

`write`/`send`/`sendto` -- does not support `SNDLOWAT`, OOB, or the address parameter 
of `sendto`.

`close` -- does not support `SOLINGER` type functionality.

`shutdown` -- sets flags but the flags are not completely implemented.

`ioctl` -- support for `FIONREAD` (bytes available to read) and `FIONBIO` 
(set/clear non-blocking).

`setsockopt` --  `SO_SNDLOWAT`, `SO_RCVLOWAT`, `SO_SNDTIMEO`, and `SO_RCVTIMEO` are supported.
`SO_OOBINLINE` errors unless it's true. 
`SO_DEBUG`, `SO_REUSEADDR`, `SO_REUSEPORT`, `SO_KEEPALIVE` set a flag but have no other effect.



`getsockopt` -- support for `SO_TYPE`, `SO_DEBUG`, `SO_REUSEADDR`, `SO_REUSEPORT`, `SO_KEEPALIVE`, 
`SO_OOBINLINE`, `SO_SNDLOWAT`, `SO_RCVLOWAT`, `SO_SNDTIMEO`, `SO_RCVTIMEO`, `SO_NREAD`, `SO_NWRITE`

`getsockopt` -- support for `SO_OOBINLINE`, `SO_SNDLOWAT`, `SO_RCVLOWAT`, 
`SO_SNDTIMEO`, `SO_RCVTIMEO`, `SNDLOWAT` and `SNDTIMEO` set flags but don not have any 
other effect.  `OOBINLINE` is only be supported when true.


Not (yet) supported:
--------------------

- OOB (out of band) data (and will never be supported).
- raw sockets (`SOCK_RAW`)
- TCP servers (`listen`/`accept`).  Should be simple, though.
- UDP servers
