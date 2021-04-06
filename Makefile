DIRS = $(wildcard log api appgen daemon)

CC          ?= gcc
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes
OBJDIR ?= $(shell pwd)
INCLUDES=-I log

all:
	for d in $(DIRS); do \
		$(MAKE) CC=$(CC) OBJDIR="$(OBJDIR)/$$d" -C "$$d" $@ || exit 1; \
		done

c99:
	for d in $(DIRS); do \
		$(MAKE) CC=$(CC) CFLAGS="-std=c99 -D_SVID_SOURCE -D _BSD_SOURCE" OBJDIR="$(OBJDIR)/$$d" -C "$$d" all || exit 1; \
		done

static:
	for d in $(DIRS); do \
		$(MAKE) CC=$(CC) OBJDIR="$(OBJDIR)/$$d" -C "$$d" $@ || exit 1; \
		done

clean:
	for d in $(DIRS); do \
		$(MAKE) OBJDIR="$(OBJDIR)/$$d" -C "$$d" $@ || exit 1; \
		done
#	rm -f /tmp/halsub* /tmp/halpub*

install: 
	mkdir -p $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)
