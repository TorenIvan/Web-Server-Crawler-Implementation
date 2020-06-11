SOURCES=myhttpd.c mtqueue.c mycrawler.c
OBJECTS=$(SOURCES:.c=.o)
BINS=myhttpd mycrawler

CFLAGS+=-std=gnu99 -Wall -g

LDFLAGS+=-pthread

all: $(BINS)

myhttpd: myhttpd.o mtqueue.o
mycrawler: mycrawler.o mtqueue.o

.PHONY: clean all

clean:
	$(RM) $(OBJECTS) $(BINS)
