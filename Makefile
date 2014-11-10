# Debugging on
CFLAGS= -g -O0 -c -Wall
INCLUDES=-I/usr/include -I/usr/local/include
LDFLAGS=-L/usr/lib -L/usr/local/lib
DESTDIR?=/usr/local
LIBS= -lucl
SRCS=uclcmd.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=uclcmd

all: $(SRCS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJS)

uclcmd.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(EXECUTABLE)

install: $(EXECUTABLE)
	$(INSTALL) -m0755 $(EXECUTABLE) $(DESTDIR)/bin/$(EXECUTABLE)
