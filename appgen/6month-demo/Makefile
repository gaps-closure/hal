CC          ?= gcc
AR          ?= ar
LIB_OBJ     =pnt.o gma.o float.o

OBJDIR ?= .
CFLAGS      = -O2 -Wall -Wstrict-prototypes

all: $(OBJDIR)/libfloat.a $(OBJDIR)/libgma.a slibs

static: libs

$(OBJDIR)/libgma.a: $(OBJDIR)/gma.o $(OBJDIR)/float.o
	ar rcs $@ $^

$(OBJDIR)/libfloat.a: $(OBJDIR)/float.o
	ar rcs $@ $^

$(OBJDIR)/libgma.so: $(OBJDIR)/gma.o
	$(CC) $(CFLAGS) -fPIC -shared -L$(OBJDIR) -o $@ $< -lfloat

$(OBJDIR)/libfloat.so: $(OBJDIR)/float.o
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<

$(OBJDIR)/%.o:	%.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

libs: $(OBJDIR)/libgma.a $(OBJDIR)/libfloat.a

slibs: $(OBJDIR)/libfloat.so $(OBJDIR)/libgma.so

clean:
	rm -f $(OBJDIR)/*.o $(OBJDIR)/*.a $(OBJDIR)/float $(OBJDIR)/*.so $(OBJDIR)/*~

