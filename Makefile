CFLAGS = -W -Wall -O0 -g
LDFLAGS =
LIBS = -lresolv

LIBOUT = libradiodns.a
LIBOBJ = context.o resolver.o

OUT = radiodns
OBJ = cli.o

$(OUT): $(OBJ) $(LIBOUT)
	$(CC) $(LDFLAGS) -o $(OUT) $(OBJ) $(LIBOUT) $(LIBS)

$(LIBOUT): $(LIBOBJ)
	ar rcs $(LIBOUT) $(LIBOBJ)

clean:
	rm -f $(OUT) $(OBJ)
	rm -f $(LIBOUT) $(LIBOBJ)
