CFLAGS += $(DEFINES) -v -w 
OBJS = main.o table.o driver.o s16debug.o mattach.o mconnect.o \
mread.o mwrite.o mdetach.o

TARGET = marignotti

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@



s16debug.o: s16debug.c s16debug.h

main.o: main.c marignotti.h
table.o: table.c marignotti.h
driver.o: driver.c marignotti.h net.h
mread.o: mread.c marignotti.h
mwrite.o: mwrite.c marignotti.h
mattach.o: mattach.c marignotti.h
mconnect.o: mconnect.c marignotti.h
mdetach.o: mdetach.c marignotti.h


clean:
	$(RM) *.o *.root

clobber: clean
	$(RM) $(TARGET) 

