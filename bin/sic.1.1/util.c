/* See LICENSE file for license details. */
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#ifndef __GNO__
#include <machine/endian.h>
#endif
// from freebsd 9.0.0

void *
memccpy(void *t, const void *f, int c, size_t n)
{

        if (n) {
                unsigned char *tp = t;
                const unsigned char *fp = f;
                unsigned char uc = c;
                do {
                        if ((*tp++ = *fp++) == uc)
                                return (tp);
                } while (--n != 0);
        }
        return (0);
}

static void
eprint(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(bufout, sizeof bufout, fmt, ap);
	va_end(ap);
	fprintf(stderr, "%s", bufout);
	if(fmt[0] && fmt[strlen(fmt) - 1] == ':')
		fprintf(stderr, " %s\n", strerror(errno));
	exit(1);
}

static int
dial(char *host, char *port) {
#ifdef __GNO__
	int srv;
	struct hostent *hp;
	struct sockaddr_in sin;

	hp = gethostbyname(host);
    if (hp == NULL)
		eprint("error: cannot resolve hostname '%s':", host);


    srv = socket(hp->h_addrtype, SOCK_STREAM, 0);
    if (srv < 0)
        eprint("error: socket");
        
	bzero((caddr_t)&sin, sizeof (sin));
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);

    sin.sin_port = htons(atoi(port));

	if (connect(srv, (struct __SOCKADDR *)&sin, sizeof(sin)) < 0)
		eprint("error: cannot connect to host '%s'\n", host);

    return srv;

#else
	static struct addrinfo hints;
	int srv;
	struct addrinfo *res, *r;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if(getaddrinfo(host, port, &hints, &res) != 0)
		eprint("error: cannot resolve hostname '%s':", host);
	for(r = res; r; r = r->ai_next) {
		if((srv = socket(r->ai_family, r->ai_socktype, r->ai_protocol)) == -1)
			continue;
		if(connect(srv, r->ai_addr, r->ai_addrlen) == 0)
			break;
		close(srv);
	}
	freeaddrinfo(res);
	if(!r)
		eprint("error: cannot connect to host '%s'\n", host);
	return srv;
#endif
}

#define strlcpy _strlcpy
static void
strlcpy(char *to, const char *from, int l) {
	memccpy(to, from, '\0', l);
	to[l-1] = '\0';
}

static char *
eat(char *s, int (*p)(int), int r) {
	while(s != '\0' && p(*s) == r)
		s++;
	return s;
}

static char*
skip(char *s, char c) {
	while(*s != c && *s != '\0')
		s++;
	if(*s != '\0')
		*s++ = '\0';
	return s;
}

static void
trim(char *s) {
	char *e;

	e = s + strlen(s) - 1;
	while(isspace(*e) && e > s)
		e--;
	*(e + 1) = '\0';
}
