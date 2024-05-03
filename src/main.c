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
} Move;

/* State-space Tree */
typedef struct Sst {
  Move *move;
  char **initB;
  int pl;
  int nChild;
  int sChild;
  struct Sst **children;
} Sst;

typedef struct {
  char lastBef;
  char lastMid;
  char lastAft;
} History;

static Sst *sst(Move *m) {
  Sst *t = malloc(sizeof(Sst));
  t->move = m;
  t->initB = NULL;
  t->pl = -1;
  t->nChild = 0;
  t->sChild = 16;
  t->children = malloc(t->sChild * sizeof(Sst *));
  return t;
}

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

static void remember(History *h, char **b, Move *p) {
  h->lastBef = b[p->x0][p->y0];
  h->lastMid = b[p->mx][p->my];
  h->lastAft = b[p->x1][p->y1];
}

static void undo(char **b, Move *p, History *h) {
  char bef = h->lastBef, mid = h->lastMid, aft = h->lastAft;
  remember(h, b, p);
  b[p->x0][p->y0] = bef;
  b[p->mx][p->my] = mid;
  b[p->x1][p->y1] = aft;
}

static void move(char **b, char *colors, Move *p, History *h, int pl,
                 int *counts0, int *counts1) {
  char taken = b[p->mx][p->my];
  remember(h, b, p);
  b[p->mx][p->my] = colors[0];
  b[p->x1][p->y1] = b[p->x0][p->y0];
  b[p->x0][p->y0] = colors[0];
  if (pl == 0) {
    ++counts0[taken - 'A'];
  } else {
    ++counts1[taken - 'A'];
  }
}

static void switchP(int *pl, int *nUndo, int *nRedo, int *wantsRedo,
                    int *hPos) {
  char continues;
  printf("\nDo you want to play another move ('y': yes, 'n': no)? ");
  scanf(" %c", &continues);
  if (continues == 'y') {

  } else {
    *pl = (*pl + 1) % 2;
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

static void compScore(int pl, int *counts0, int *counts1, int *score0,
                      int *score1) {
  int *counts, *prevScore, min_, score;
  if (pl == 0) {
    counts = counts0;
    prevScore = score0;
  } else {
    counts = counts1;
    prevScore = score1;
  }
  /* printf("\nCounts: "); */
  /* printArr(counts, N_SKIPPER); */
  min_ = findMin(counts, N_SKIPPER);
  score = min_;
  *prevScore = score;
}

static void save(char **b, size_t n, int pl) {
  char *buf = calloc((size_t)(n * n + 2), sizeof(char));
  size_t i, j, k = 0;
  FILE *outf;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      buf[k++] = b[i][j];
    }
  }
  buf[k++] = (char)pl + '0';
  printf("\n");
  outf = fopen("skippity.txt", "w");
  fprintf(outf, "%s", buf);
  free(buf);
}

static int inBoard(int x, int y, size_t n) {
  int n2 = (int)n;
  return x >= 0 && x < n2 && y >= 0 & y < n2;
}

static int canMove(char **b, size_t n, char *colors, int i, int j, int di,
                   int dj) {
  return inBoard(i + di, j + dj, n) && inBoard(i + 2 * di, j + 2 * dj, n) &&
         b[i + di][j + dj] != colors[0] &&
         b[i + 2 * di][j + 2 * dj] == colors[0];
}

static int gameEnds(char **b, size_t n, char *colors) {
  int ends = 1;
  int i, j;
  for (i = 0; i < (int)n && ends; ++i) {
    for (j = 0; j < (int)n && ends; ++j) {
      if (b[i][j] != colors[0]) {
        if (canMove(b, n, colors, i, j, 1, 0) ||
            canMove(b, n, colors, i, j, 0, 1) ||
            canMove(b, n, colors, i, j, -1, 0) ||
            canMove(b, n, colors, i, j, 0, -1)) {
          ends = 0;
        }
      }
    }
  }
  return ends;
}

static void setMiddle(Move *m) {
  m->mx = (m->x0 + m->x1) / 2;
  m->my = (m->y0 + m->y1) / 2;
}

static void playHuman(char **b, size_t n, char *colors, int pl) {
  int ended = 0, nUndo = 0, nRedo = 0, hPos = 0, redoes, undoes, canUndo,
      wantsRedo = 1, score0, score1, i;
  Move p;
  char input;
  int *counts0 = calloc(N_SKIPPER, sizeof(int));
  int *counts1 = calloc(N_SKIPPER, sizeof(int));
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
          switchP(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
        }
        nRedo += 1;
      } else {
        wantsRedo = 0;
        switchP(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
      }
    } else {
      printf("\nDo you want to save and exit ('y': yes)? ");
      scanf(" %c", &input);
      if (input == 'y') {
        save(b, n, pl);
        exit(EXIT_SUCCESS);
      }
      printf("\nPlayer %c, enter your move (x0, y0, x1, y1): ", pl + '0');
      scanf(" %d %d %d %d", &p.x0, &p.y0, &p.x1, &p.y1);
      /* printf("\nMove: (%d, %d), (%d, %d)\n", p.x0, p.y0, p.x1, p.y1); */
      setMiddle(&p);
      /* printf("\nMiddle x: %d, Middle y: %d\n", p.mx, p.my); */
      move(b, colors, &p, &h, pl, counts0, counts1);
      printBoard(b, n);
      compScore(pl, counts0, counts1, &score0, &score1);
      printf("\nScore of pl 0: %d, 1: %d\n", score0, score1);
      if (gameEnds(b, n, colors)) {
        if (score0 == score1) {
          int min0 = findMin(counts0, N_SKIPPER),
              min1 = findMin(counts1, N_SKIPPER);
          int escore0 = 0, escore1 = 0;
          for (i = 0; i < N_SKIPPER; ++i) {
            escore0 += counts0[i] - min0;
          }
          for (i = 0; i < N_SKIPPER; ++i) {
            escore1 += counts1[i] - min1;
          }
          printf("\nExtra score of pl 0: %d\n", escore0);
          printf("\nExtra score of pl 1: %d\n", escore1);
          if (escore0 == escore1) {
            printf("\nGame ends in a tie\n");
          } else if (escore0 > escore1) {
            printf("\nPlayer 0 wins\n");
          } else {
            printf("\nPlayer 1 wins\n");
          }
        } else if (score0 > score1) {
          printf("\nPlayer 0 wins\n");
        } else {
          printf("\nPlayer 1 wins\n");
        }
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
          switchP(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
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

static void printMove(Move *m) {
  printf("[(%d, %d), (%d, %d), (%d, %d)]\n", m->x0, m->y0,
         m->mx, m->my, m->x1, m->y1);
}

static void printSubsst(Sst *r) {
  if (r) {
    int i;
    printMove(r->move);
    for (i = 0; i < r->nChild; ++i) {
      printf("\t");
      printSubsst(r->children[i]);
    }
  }
}

static void printSst(Sst *r) {
  printf("\nSst:\n");
  printSubsst(r);
}

static Move *makeMove(int x0, int y0, int x1, int y1) {
  Move *m = malloc(sizeof(Move));
  m->x0 = x0;
  m->y0 = y0;
  m->x1 = x1;
  m->y1 = y1;
  setMiddle(m);
  return m;
}

static void insert(Sst *r, Move *m, int pl) {
  Sst **child = &(r->children[r->nChild++]);
  if (r->nChild >= r->sChild) {
    r->sChild *= 2;
    r->children = realloc(r->children, r->sChild);
  }
  *child = sst(m);
  (*child)->pl = pl;
  (*child)->initB = r->initB;
}

static void freeSst(Sst *r) {
    int i;
    for (i = 0; i < r->nChild; ++i) {
        freeSst(r->children[i]);
    }
    free(r->children);
    free(r->move);
    free(r);
}

static void testSst() {
  Sst *r;
  int pl;
  r = sst(makeMove(0, 2, 2, 2));
  pl = 0;
  insert(r, makeMove(0, 3, 2, 3), pl);
  printSst(r);
  freeSst(r);
  exit(EXIT_SUCCESS);
}

int main() {
  size_t n;
  char **b = NULL;
  size_t i, j;
  char input;
  char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
  int mode;
  int pl = 0;
  testSst();
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
    pl = buf[k - 1] - '0';
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
    playHuman(b, n, colors, pl);
  } else {
  }
  for (i = 0; i < n; ++i) {
    free(b[i]);
  }
  free(b);
  return 0;
}
