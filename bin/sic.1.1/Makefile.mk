DEFINES += -DVERSION=1.1 #-D__STACK_CHECK__=1
CFLAGS += $(DEFINES) -v -w 
LDFLAGS += -l/usr/lib/libnetdb

OBJS = sic.o


sic: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@


sic.o: sic.c util.c
