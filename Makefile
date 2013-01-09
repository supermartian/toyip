#
# dhcpfucker - send out a given times of dhcp requests
# Author: Wen Yuzhong wenyuzhong@tp-link.net
#

SRCS=$(shell ls *\.c)
OBJS=$(subst .c,.o,$(SRCS))
APPS= toyip

LDFLAGS += -lpthread

all: $(OBJS)
	$(CC) -o toyip $(OBJS) $(LDFLAGS)

$(OBJS): %.o:%.c
	$(CC) -ggdb -c $(CFLAGS) $< -o $@

debug: $(OBJS)
	$(CC) -ggdb -o toyip $(OBJS) $(LDFLAGS)

clean:
	rm $(APPS) $(OBJS)
