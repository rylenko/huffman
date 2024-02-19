.POSIX:

include config.mk

SRC = src/main.c src/huffman.c src/utils.c
OBJ = $(SRC:.c=.o)

huffman: $(OBJ)
	$(CC) -o $@ $(OBJ) $(CFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

src/main.o: src/consts.h src/errors.h src/huffman.h
src/huffman.o: src/consts.h src/errors.h src/huffman.h src/utils.h
src/utils.o: src/utils.h

clean:
	rm -f huffman $(OBJ)

install: huffman
	mkdir -p $(PREFIX)/bin
	cp -f $< $(PREFOX)/bin/$<
	chmod 755 $(PREFIX)/bin/$<

uninstall:
	rm -f $(PREFIX)/bin/huffman

.PHONY: huffman clean install uninstall
