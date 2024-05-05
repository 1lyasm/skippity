debug:
	clang -g -fsanitize=address -Weverything src/main.c -o main
fast:
	clang -O3 src/main.c -lm -o main
run:
	./main
