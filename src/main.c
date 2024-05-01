#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static void printBoard(char **board, size_t n) {
    size_t i, j;
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            printf("%c ", board[i][j]);
        }
        printf("\n");
    }
}

int main() {
    size_t n;
    char **board;
    size_t i, j;
    int skipperCount = 5;
    char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
    srand((unsigned)time(NULL));
    printf("Enter board size: ");
    scanf(" %lu", &n);
    board = malloc(n * sizeof(char *));
    for (i = 0; i < n; ++i) {
        board[i] = malloc(n * sizeof(char));
    }
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            board[i][j] = colors[rand() % skipperCount + 1];
        }
    }
    for (i = n / 2 - 1; i <= n / 2; ++i) {
        for (j = n / 2 - 1; j <= n / 2; ++j) {
            board[i][j] = colors[0];
        }
    }
    printBoard(board, n);
    for (i = 0; i < n; ++i) {
        free(board[i]);
    }
    free(board);
    return 0;
}

