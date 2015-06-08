# Debugging on
INCLUDES=-I/usr/include -I/usr/local/include
LDFLAGS=-L/usr/lib -L/usr/local/lib
CFLAGS= -g -O0 -Wall $(INCLUDES)
DESTDIR?=/usr/local
LIBS= -lucl
SRCS=uclcmd.c uclcmd_common.c uclcmd_get.c uclcmd_merge.c uclcmd_output.c \
	uclcmd_parse.c uclcmd_remove.c uclcmd_set.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=uclcmd

all: $(SRCS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJS)

clean:
	rm -f *.o $(EXECUTABLE)

install: $(EXECUTABLE)
	$(INSTALL) -m0755 $(EXECUTABLE) $(DESTDIR)/bin/$(EXECUTABLE)
