DIRS = $(wildcard zc log api appgen daemon)

CC          ?= gcc
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes
OBJDIR ?= $(shell pwd)
INCLUDES=-I log

all:
	for d in $(DIRS); do \
		$(MAKE) CC=$(CC) OBJDIR="$(OBJDIR)/$$d" -C "$$d" $@ || exit 1; \
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
	install -m 755 zc/zc $(INSTALLPATH)
	install -m 755 hal   $(INSTALLPATH)
