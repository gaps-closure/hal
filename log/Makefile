CC          ?= gcc
CFLAGS      = -O2 -Wall -Wstrict-prototypes #-g -pg

OBJDIR ?= .

all:  $(OBJDIR)/log.o

static: all

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) -fpic -c $< $a

clean:
	rm -f $(OBJDIR)/*.o
