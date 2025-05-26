build: algo game

all: indent build test

indent:
	sed "s/\r//g" -i *.h *.c
	indent -npsl -nut *.h *.c

algo: algo.h algo.c algo_tests.c
	gcc -std=c99 -Wall -Wextra -pedantic -g -O2 $^ -o $@

game: algo.h algo.c game.c
	gcc -std=c99 -Wall -Wextra -pedantic -g -O2 $^ -o $@

test: algo
	valgrind -q --leak-check=full ./$<

clean:
	rm -f algo game *~

run : build test
