MariGNOtti

MariGNOtti is a network driver for GNO/ME 2.0.6.  It provides a translation layer between BSD sockets and Marinetti. 

Architecture:

BSD sockets are, by default, blocking whereas everything Marinetti does (other than the inital network connection) is non-blocking.  To handle blocking behavior, each socket has a semaphore.  The main thread performs all the polling and signals the other threads when they can proceed.

Supported:

socket(2) -- but only for PF_UNIX, SOCK_STREAM, TCP.

connect(2) -- but non-blocking connects are not yet supported.

bind(2) -- currently a no-op since the old whois(1) used it.

read(2) -- supports blocking, non-blocking, timeouts, and RCVLOWAT.

write(2) -- currently no SNDLOWAT type functionality.

close(2) -- currently no SOLINGER type functionality.

shutdown(2) -- set flags but the flags have no effect.

ioctl(2) -- support for FIONREAD (bytes available to read) and FIONBIO (set/clear non-blocking).

getsockopt(2) -- support for SO_OOBINLINE, SO_SNDLOWAT, SO_RCVLOWAT, SO_SNDTIMEO, SO_RCVTIMEO. SNDLOWAT and SNDTIMEO set flags but don't have any other effect.  OOBINLINE will only be supported when true.

getsockopt(2) -- support for SO_TYPE, SO_OOBINLINE, SO_SNDLOWAT, SO_RCVLOWAT, SO_SNDTIMEO

send(2)/sendto(2) -- flags and address are ignored (ie, same as write(2))
recv(2)/redcvfrom(2) -- flags and address are ignored. (ie, same as read(2))

Since UDP is not supported, you'll want to put some values in your /etc/hosts file.  You'll also need to create an /etc/services file.

whois works.
ftp does not yet work.
