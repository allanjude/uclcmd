# Debugging on
CFLAGS?=-g -O0
CFLAGS+=-Wall $(INCLUDES)
CFLAGS+=`pkg-config --cflags libucl`
LIBS+=`pkg-config --libs libucl`
PREFIX?=/usr/local
SRCS=uclcmd.c uclcmd_common.c uclcmd_get.c uclcmd_merge.c \
	uclcmd_output.c uclcmd_parse.c uclcmd_remove.c uclcmd_set.c
OBJS=$(SRCS:.c=.o)
EXECUTABLE=uclcmd

all: $(SRCS) $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
	$(CC) $(LDFLAGS) $(LIBS) -o $(EXECUTABLE) $(OBJS)

clean:
	rm -f *.o $(EXECUTABLE)

install: $(EXECUTABLE)
	$(INSTALL) -m0755 $(EXECUTABLE) $(DESTDIR)$(PREFIX)/bin/$(EXECUTABLE)
