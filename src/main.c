#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_SKIPPER 5

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

static void move(char **b, char *colors, Points *p, History *h, int player,
                 int *counts0, int *counts1) {
  char taken = b[p->mx][p->my];
  remember(h, b, p);
  b[p->mx][p->my] = colors[0];
  b[p->x1][p->y1] = b[p->x0][p->y0];
  b[p->x0][p->y0] = colors[0];
  if (player == 0) {
    ++counts0[taken - 'A'];
  } else {
    ++counts1[taken - 'A'];
  }
}

static void switchP(int *player, int *nUndo, int *nRedo, int *wantsRedo,
                    int *hPos) {
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

static int findMin(int *arr, int n) {
  int min_ = INT_MAX, i;
  for (i = 0; i < n; ++i) {
    if (arr[i] < min_) {
      min_ = arr[i];
    }
  }
  return min_;
}

static void printArr(int *counts, int n) {
  int i;
  printf("\n");
  for (i = 0; i < n; ++i) {
    printf("%d ", counts[i]);
  }
  printf("\n");
}

static int compScore(int player, int *counts0, int *counts1) {
  int *counts, min_, i, score;
  if (player == 0) {
    counts = counts0;
  } else {
    counts = counts1;
  }
  /* printf("\nCounts: "); */
  /* printArr(counts, N_SKIPPER); */
  min_ = findMin(counts, N_SKIPPER);
  score = min_;
  for (i = 0; i < N_SKIPPER; ++i) {
    score += counts[i] - min_;
  }
  return score;
}

static void save(char **b, size_t n, int player) {
  char *buf = calloc((size_t)(n * n + 2), sizeof(char));
  size_t i, j, k = 0;
  FILE *outf;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      buf[k++] = b[i][j];
    }
  }
  buf[k++] = (char)player + '0';
  printf("\n");
  outf = fopen("skippity.txt", "w");
  fprintf(outf, "%s", buf);
  free(buf);
}

static int gameEnds(char **b, size_t n) {
    return 0;
}

static void playHuman(char **b, size_t n, char *colors, int player) {
  int ended = 0;
  Points p;
  int nUndo = 0;
  int nRedo = 0;
  int hPos = 0;
  int redoes;
  int undoes;
  char input;
  int canUndo;
  int wantsRedo = 1;
  int *counts0 = calloc(N_SKIPPER, sizeof(int));
  int *counts1 = calloc(N_SKIPPER, sizeof(int));
  int score;
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
      printf("\nDo you want to save and exit ('y': yes)? ");
      scanf(" %c", &input);
      if (input == 'y') {
        save(b, n, player);
        exit(EXIT_SUCCESS);
      }
      printf("\nPlayer %c, enter your move (x0, y0, x1, y1): ", player + '0');
      scanf(" %d %d %d %d", &p.x0, &p.y0, &p.x1, &p.y1);
      /* printf("\nMove: (%d, %d), (%d, %d)\n", p.x0, p.y0, p.x1, p.y1); */
      p.mx = (p.x0 + p.x1) / 2;
      p.my = (p.y0 + p.y1) / 2;
      /* printf("\nMiddle x: %d, Middle y: %d\n", p.mx, p.my); */
      move(b, colors, &p, &h, player, counts0, counts1);
      printBoard(b, n);
      score = compScore(player, counts0, counts1);
      printf("\nScore of player %d: %d\n", player, score);
      if (gameEnds(b, n)) {
        ended = 1;
      } else {
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
  free(counts0);
  free(counts1);
}

int main() {
  size_t n;
  char **b = NULL;
  size_t i, j;
  char input;
  char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
  int mode;
  int player = 0;
  srand((unsigned)time(NULL));
  printf("Do you want to continue the previous game ('y': yes)? ");
  scanf(" %c", &input);
  if (input == 'y') {
    FILE *inf = fopen("skippity.txt", "r");
    char *buf = malloc(1024 * sizeof(char));
    size_t k;
    fgets(buf, 1024, inf);
    fclose(inf);
    printf("\nBuf: %s\n", buf);
    k = strlen(buf);
    n = (size_t)(sqrt((double)(k - 1)));
    printf("\nN: %lu\n", n);
    b = malloc(n * sizeof(char *));
    for (i = 0; i < n; ++i) {
      b[i] = malloc(n * sizeof(char));
    }
    for (i = 0; i < k - 1; ++i) {
      /* printf("i / n: %d\n", i / n); */
      b[i / n][i % n] = buf[i];
    }
    player = buf[k - 1] - '0';
    free(buf);
  } else {
    printf("\nEnter board size: ");
    scanf(" %lu", &n);
    b = malloc(n * sizeof(char *));
    for (i = 0; i < n; ++i) {
      b[i] = malloc(n * sizeof(char));
    }
    for (i = 0; i < n; ++i) {
      for (j = 0; j < n; ++j) {
        b[i][j] = colors[rand() % N_SKIPPER + 1];
      }
    }
    for (i = n / 2 - 1; i <= n / 2; ++i) {
      for (j = n / 2 - 1; j <= n / 2; ++j) {
        b[i][j] = colors[0];
      }
    }
  }
  printf("\nBoard: \n");
  printBoard(b, n);
  printf("\nEnter game mode (0: human, 1: computer): ");
  scanf(" %d", &mode);
  if (mode == 0) {
    playHuman(b, n, colors, player);
  } else {
  }
  for (i = 0; i < n; ++i) {
    free(b[i]);
  }
  free(b);
  return 0;
}
