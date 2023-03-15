CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c11

mark4: 4/prog.c
	$(CC) $(CFLAGS) -o mark4 4/prog.c

all: mark4
.DEFAULT_GOAL := all

clean:
	rm -f mark4