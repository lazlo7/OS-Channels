CC=gcc
CFLAGS=-Wall -Wextra -Werror -pedantic -std=c11

mark4: 4/prog.c
	$(CC) $(CFLAGS) -o mark4 4/prog.c

mark5: 5/prog.c
	$(CC) $(CFLAGS) -o mark5 5/prog.c

run_tests_4: mark4
	./mark4 test/1/in1 test/1/in2 test/1/out1 test/1/out2
	./mark4 test/2/in1 test/2/in2 test/2/out1 test/2/out2
	./mark4 test/big/in1 test/big/in2 test/big/out1 test/big/out2
	./mark4 test/distinct/in1 test/distinct/in2 test/distinct/out1 test/distinct/out2
	./mark4 test/equal/in1 test/equal/in2 test/equal/out1 test/equal/out2
	./mark4 test/lorem/in1 test/lorem/in2 test/lorem/out1 test/lorem/out2

run_tests_5: mark5
	./mark5 test/1/in1 test/1/in2 test/1/out1 test/1/out2
	./mark5 test/2/in1 test/2/in2 test/2/out1 test/2/out2
	./mark5 test/big/in1 test/big/in2 test/big/out1 test/big/out2
	./mark5 test/distinct/in1 test/distinct/in2 test/distinct/out1 test/distinct/out2
	./mark5 test/equal/in1 test/equal/in2 test/equal/out1 test/equal/out2
	./mark5 test/lorem/in1 test/lorem/in2 test/lorem/out1 test/lorem/out2

clean:
	rm -f mark4