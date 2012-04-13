CFLAGS += $(DEFINES) -v -w 
OBJS = marignotti.o
TARGET = marignotti

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

marignotti.o: marignotti.c net.h

pull:
	gopher gopher://192.168.1.117:7070/0/marignotti.c > marignotti.c

clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) $(TARGET) 

