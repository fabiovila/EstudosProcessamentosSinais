CC=gcc
CFLAGS=-g -Wall $(shell sdl2-config --cflags)
LDFLAGS=$(shell sdl2-config --libs) -lSDL2_mixer -lm -lfftw3

PROGRAMS=$(basename $(wildcard *.c))

%: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

all: $(PROGRAMS)

check:
	valgrind --leak-check=yes $(PROGRAMS)

clean:
	rm -f $(PROGRAMS) *.o

new: clean all
