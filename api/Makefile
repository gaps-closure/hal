CC          ?= gcc
AR          ?= ar
CFLAGS      ?= -O2 -Wall -Wstrict-prototypes
LDLIBS       =-lzmq
OBJDIR       =.

ifeq ($(CC),cc)
CC=gcc
endif

ifneq ($(STATICZMQ),)
LDLIBS = $(LDFLAGS)
endif

all: $(OBJDIR)/libxdcomms.a $(OBJDIR)/libxdcomms.so

static: $(OBJDIR)/libxdcomms.a

$(OBJDIR)/../log/log.o: ../log/log.c
	$(CC) $(CFLAGS) -fpic -c ../log/log.c -o $@

$(OBJDIR)/libxdcomms.a: $(OBJDIR)/../log/log.o $(OBJDIR)/xdcomms.o 
	$(AR) rcs $@ $^ 

$(OBJDIR)/libxdcomms.so: $(OBJDIR)/../log/log.o $(OBJDIR)/xdcomms.o
	$(info $(CC) $(CFLAGS) $(STATICZMQ) -shared -o $@ $^ $(LDLIBS))
	$(CC) $(CFLAGS) $(STATICZMQ) -shared -o $@ $^ $(LDLIBS)

$(OBJDIR)/xdcomms.o:  xdcomms.c
	$(CC) $(CFLAGS) -fpic -c $< -o $@

libs: $(OBJDIR)/libxdcomms.a $(OBJDIR)/libxdcomms.so

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.a $(OBJDIR)/*.so


