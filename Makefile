all:
	clang -g -fsanitize=address -Wall src/main.c -o main
run:
	./main
