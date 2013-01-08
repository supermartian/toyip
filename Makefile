#
# dhcpfucker - send out a given times of dhcp requests
# Author: Wen Yuzhong wenyuzhong@tp-link.net
#

SRCS=$(shell ls *\.c)
OBJS=$(subst .c,.o,$(SRCS))
APPS=$(subst .c,,$(SRCS))

LDFLAGS += -lpthread

all: $(APPS)

clean:
	rm $(APPS)
