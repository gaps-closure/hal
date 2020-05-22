CC          ?= gcc
AR          ?= ar
CFLAGS      = -O2 -Wall -Wstrict-prototypes -L.
LIB_OBJ     = pnt.o 

OBJDIR ?= .

all: $(OBJDIR)/libpnt.a $(OBJDIR)/libpnt.so

static: $(OBJDIR)libpnt.a

$(OBJDIR)/libpnt.a: $(OBJDIR)/pnt.o
	$(AR) rcs $@  $^

$(OBJDIR)/libpnt.so: $(OBJDIR)/pnt.o
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $< 

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.a $(OBJDIR)/*.so $(OBJDIR)/*~

