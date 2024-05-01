#include <stdio.h>
#include <stdlib.h>

int main() {
    size_t boardSize;
    int **board;
    size_t i;
    printf("Enter board size: ");
    scanf(" %lu", &boardSize);
    board = malloc(boardSize * sizeof(int *));
    for (i = 0; i < boardSize; ++i) {
        board[i] = malloc(boardSize * sizeof(int));
    }


    for (i = 0; i < boardSize; ++i) {
        free(board[i]);
    }
    free(board);
    return 0;
}

