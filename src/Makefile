CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c11

mark4: 4/prog.c
	$(CC) $(CFLAGS) -o mark4 4/prog.c

mark5: 5/prog.c
	$(CC) $(CFLAGS) -o mark5 5/prog.c

mark6: 6/prog.c
	$(CC) $(CFLAGS) -o mark6 6/prog.c

mark7: 7/prog.c
	$(CC) $(CFLAGS) -o mark7 7/prog.c

mark8: 8/reader-writer.c 8/data-handler.c
	$(CC) $(CFLAGS) -o mark8-rw 8/reader-writer.c
	$(CC) $(CFLAGS) -o mark8-h 8/data-handler.c

run_tests_4: mark4
	./mark4 test/in/1/1 test/in/1/2 test/out/mark4/1/1 test/out/mark4/1/2
	./mark4 test/in/2/1 test/in/2/2 test/out/mark4/2/1 test/out/mark4/2/2
	./mark4 test/in/big/1 test/in/big/2 test/out/mark4/big/1 test/out/mark4/big/2
	./mark4 test/in/distinct/1 test/in/distinct/2 test/out/mark4/distinct/1 test/out/mark4/distinct/2
	./mark4 test/in/equal/1 test/in/equal/2 test/out/mark4/equal/1 test/out/mark4/equal/2
	./mark4 test/in/lorem/1 test/in/lorem/2 test/out/mark4/lorem/1 test/out/mark4/lorem/2

run_tests_5: mark5
	./mark5 test/in/1/1 test/in/1/2 test/out/mark5/1/1 test/out/mark5/1/2
	./mark5 test/in/2/1 test/in/2/2 test/out/mark5/2/1 test/out/mark5/2/2
	./mark5 test/in/big/1 test/in/big/2 test/out/mark5/big/1 test/out/mark5/big/2
	./mark5 test/in/distinct/1 test/in/distinct/2 test/out/mark5/distinct/1 test/out/mark5/distinct/2
	./mark5 test/in/equal/1 test/in/equal/2 test/out/mark5/equal/1 test/out/mark5/equal/2
	./mark5 test/in/lorem/1 test/in/lorem/2 test/out/mark5/lorem/1 test/out/mark5/lorem/2

run_tests_6: mark6
	./mark6 test/in/1/1 test/in/1/2 test/out/mark6/1/1 test/out/mark6/1/2
	./mark6 test/in/2/1 test/in/2/2 test/out/mark6/2/1 test/out/mark6/2/2
	./mark6 test/in/big/1 test/in/big/2 test/out/mark6/big/1 test/out/mark6/big/2
	./mark6 test/in/distinct/1 test/in/distinct/2 test/out/mark6/distinct/1 test/out/mark6/distinct/2
	./mark6 test/in/equal/1 test/in/equal/2 test/out/mark6/equal/1 test/out/mark6/equal/2
	./mark6 test/in/lorem/1 test/in/lorem/2 test/out/mark6/lorem/1 test/out/mark6/lorem/2

run_tests_7: mark7
	./mark7 test/in/1/1 test/in/1/2 test/out/mark7/1/1 test/out/mark7/1/2
	./mark7 test/in/2/1 test/in/2/2 test/out/mark7/2/1 test/out/mark7/2/2
	./mark7 test/in/big/1 test/in/big/2 test/out/mark7/big/1 test/out/mark7/big/2
	./mark7 test/in/distinct/1 test/in/distinct/2 test/out/mark7/distinct/1 test/out/mark7/distinct/2
	./mark7 test/in/equal/1 test/in/equal/2 test/out/mark7/equal/1 test/out/mark7/equal/2
	./mark7 test/in/lorem/1 test/in/lorem/2 test/out/mark7/lorem/1 test/out/mark7/lorem/2

clean:
	rm -f mark4 mark5 mark6 mark7 mark8-rw mark8-h unhandled_1.fifo unhandled_2.fifo handled_1.fifo handled_2.fifo