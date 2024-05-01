#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

typedef struct {
    int x0;
    int y0;
    int x1;
    int y1;
    int mx;
    int my;
} Points;

typedef struct {
    char lastBef;
    char lastMid;
    char lastAft;
} History;

static void printBoard(char **b, size_t n) {
    size_t i, j;
    printf("\n");
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            printf("%c ", b[i][j]);
        }
        printf("\n");
    }
}

static void remember(History *h, char **b, Points *p) {
    h->lastBef = b[p->x0][p->y0];
    h->lastMid = b[p->mx][p->my];
    h->lastAft = b[p->x1][p->y1];
}

static void undo(char **b, Points *p, History *h) {
    char bef = h->lastBef, mid = h->lastMid, aft = h->lastAft;
    remember(h, b, p);
    b[p->x0][p->y0] = bef;
    b[p->mx][p->my] = mid;
    b[p->x1][p->y1] = aft;
}

static void move(char **b, char *colors, Points *p, History *h) {
    remember(h, b, p);
    b[p->mx][p->my] = colors[0];
    b[p->x1][p->y1] = b[p->x0][p->y0];
    b[p->x0][p->y0] = colors[0];
}


static void switchP(int *player, int *nUndo, int *nRedo, int *wantsRedo, int *hPos) {
    char continues;
    printf("\nDo you want to play another move ('y': yes, 'n': no)? ");
    scanf(" %c", &continues);
    if (continues == 'y') {

    } else {
        *player = (*player + 1) % 2;
    }
    *nUndo = 0;
    *nRedo = 0;
    *wantsRedo = 1;
    *hPos = 0;
}

static void playHuman(char **b, size_t n, char *colors) {
    int player = 0;
    int ended = 0;
    Points p;
    int nUndo = 0;
    int nRedo = 0;
    int hPos = 0;
    char middleColor = '\0';
    int redoes;
    int undoes;
    char input;
    int canUndo;
    int wantsRedo = 1;
    History h;
    while (ended == 0) {
        canUndo = 0;
        redoes = 0;
        undoes = 0;
        if (hPos <= -1 && wantsRedo) {
            printf("\nRedo? ('y': yes, 'n': no): ");
            scanf(" %c", &input);
            redoes = input == 'y';
            if (redoes) {
                undo(b, &p, &h);
                ++hPos;
                printBoard(b, n);
                if (nRedo >= 1) {
                    switchP(&player, &nUndo, &nRedo, &wantsRedo, &hPos);
                }
                nRedo += 1;
            } else {
                wantsRedo = 0;
                switchP(&player, &nUndo, &nRedo, &wantsRedo, &hPos);
            }
        } else {
            printf("\nPlayer %c, enter your move (x0, y0, x1, y1): ", player + '0');
            scanf(" %d %d %d %d", &p.x0, &p.y0, &p.x1, &p.y1);
            printf("\nMove: (%d, %d), (%d, %d)\n", p.x0, p.y0, p.x1, p.y1);
            p.mx = (p.x0 + p.x1) / 2;
            p.my = (p.y0 + p.y1) / 2;
            middleColor = b[p.mx][p.my];
            printf("\nMiddle x: %d, Middle y: %d\n", p.mx, p.my);
            move(b, colors, &p, &h);
            printBoard(b, n);
            if (nUndo <= 0) {
                printf("\nUndo ('y': yes, 'n': no)? ");
                scanf(" %c", &input);
                undoes = input == 'y';
                if (undoes && hPos <= -1) {
                    printf("\nCan not undo\n");
                    printBoard(b, n);
                } else {
                    canUndo = 1;
                }
                printf("\nCan undo: %d\n", canUndo);
            } else {
                undoes = 0;
            }
            if (!(undoes && nUndo == 0) || !canUndo) {
                switchP(&player, &nUndo, &nRedo, &wantsRedo, &hPos);
            } else {
                undo(b, &p, &h);
                if (undoes) {
                    --hPos;
                    nUndo += 1;
                }
                printBoard(b, n);
            }
        }
    }
}

int main() {
    size_t n;
    char **b;
    size_t i, j;
    int skipperCount = 5;
    char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
    int mode;
    srand((unsigned)time(NULL));
    printf("\nEnter board size: ");
    scanf(" %lu", &n);
    b = malloc(n * sizeof(char *));
    for (i = 0; i < n; ++i) {
        b[i] = malloc(n * sizeof(char));
    }
    for (i = 0; i < n; ++i) {
        for (j = 0; j < n; ++j) {
            b[i][j] = colors[rand() % skipperCount + 1];
        }
    }
    for (i = n / 2 - 1; i <= n / 2; ++i) {
        for (j = n / 2 - 1; j <= n / 2; ++j) {
            b[i][j] = colors[0];
        }
    }
    printf("\nInitial board: \n");
    printBoard(b, n);
    printf("\nEnter game mode (0: human, 1: computer): ");
    scanf(" %d", &mode);
    if (mode == 0) {
        playHuman(b, n, colors);
    } else {
    }
    for (i = 0; i < n; ++i) {
        free(b[i]);
    }
    free(b);
    return 0;
}

