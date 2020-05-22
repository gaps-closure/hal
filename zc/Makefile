LDLIBS=-lzmq
COMPATLIBS  = -static ../x86_64_prebuild/libzmq.a -Wl,--allow-multiple-definition -Wl,-Bstatic -lsodium -lunwind -llzma  -lc -lstdc++ -lpthread -ldl
OBJDIR=.

all: $(OBJDIR)/zc

$(OBJDIR)/zc: $(OBJDIR)/zc.o

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

static: $(OBJDIR)/zc.o
	$(CC) -o $(OBJDIR)/zc $(OBJDIR)/zc.o $(COMPATLIBS)

clean:
	rm -f $(OBJDIR)/zc $(OBJDIR)/*.o
