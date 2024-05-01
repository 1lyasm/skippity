all:
	clang -g -fsanitize=address -Weverything src/main.c -o main
run:
	./main
