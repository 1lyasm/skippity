debug:
	clang -g -fsanitize=address -Wall src/main.c -o main
fast:
	clang -O3 src/main.c -lm -o main
run:
	./main
