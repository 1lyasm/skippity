#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_SKIPPER 5
#define FALSE 0
#define TRUE 1

typedef enum {
    Exists, Filled, Added
} AddState;

typedef struct {
    AddState state;
    size_t index;
} AddRes;

typedef struct {
    size_t x0;
    size_t y0;
    size_t x1;
    size_t y1;
    size_t mx;
    size_t my;
} Move;

/* State-space Tree */
typedef struct Sst {
    Move *move;
    char **b;
    size_t n;
    int pl;
    size_t nChild;
    size_t sChild;
    struct Sst **children;
} Sst;

typedef struct {
    char lastBef;
    char lastMid;
    char lastAft;
} History;

static void moveNoMem(char **b, char *colors, Move *p) {
  b[p->mx][p->my] = colors[0];
  b[p->x1][p->y1] = b[p->x0][p->y0];
  b[p->x0][p->y0] = colors[0];
}

static void printBoard(char **b, size_t n) {
  size_t i, j;
  printf("\n  ");
  for (i = 0; i < n; ++i) {
    printf("%d ", (int) i);
  }
  printf("\n");
  for (i = 0; i < n; ++i) {
    printf("%d ", (int) i);
    for (j = 0; j < n; ++j) {
      printf("%c ", b[i][j]);
    }
    printf("\n");
  }
}

static char **copyB(char **b, size_t n) {
  size_t i, j;
  char **newB = malloc(n * sizeof(char *));
  for (i = 0; i < n; ++i) {
    newB[i] = malloc(n * sizeof(char));
    for (j = 0; j < n; ++j) {
      newB[i][j] = b[i][j];
    }
  }
  return newB;
}

static Sst *sst(Move *m, char **b, size_t n, int pl) {
  Sst *t = malloc(sizeof(Sst));
  t->move = m;
  t->b = b;
  t->n = n;
  t->pl = pl;
  t->nChild = 0;
  t->sChild = 16;
  t->children = malloc(t->sChild * sizeof(Sst *));
  return t;
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
  char *buf = calloc((size_t) (n * n + 2), sizeof(char));
  size_t i, j, k = 0;
  FILE *outf;
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      buf[k++] = b[i][j];
    }
  }
  buf[k++] = (char) pl + '0';
  printf("\n");
  outf = fopen("skippity.txt", "w");
  fprintf(outf, "%s", buf);
  free(buf);
}

static int inBoard(int x, int y, size_t n) {
  int n2 = (int) n;
  return x >= 0 && x < n2 && y >= 0 & y < n2;
}

static int canMove(char **b, size_t n, char *colors, size_t i, size_t j, int di,
                   int dj) {
  return inBoard((int) i + di, (int) j + dj, n) &&
         inBoard((int) i + 2 * di, (int) j + 2 * dj, n) &&
         b[(int) i + di][(int) j + dj] != colors[0] &&
         b[(int) i + 2 * di][(int) j + 2 * dj] == colors[0];
}

static int gameEnds(char **b, size_t n, char *colors) {
  int ends = 1;
  size_t i, j;
  for (i = 0; i < n && ends; ++i) {
    for (j = 0; j < n && ends; ++j) {
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

static void playWithHuman(char **b, size_t n, char *colors, int pl) {
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
        nRedo += 1;
        if (nRedo >= 1) {
          switchP(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
        }
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
      scanf(" %llu %llu %llu %llu", &p.x0, &p.y0, &p.x1, &p.y1);
      setMiddle(&p);
      move(b, colors, &p, &h, pl, counts0, counts1);
      printBoard(b, n);
      compScore(pl, counts0, counts1, &score0, &score1);
      printf("\nScore of player 0: %d, 1: %d\n", score0, score1);
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
  printf("[ %llu, %llu -> %llu, %llu ]", m->x0, m->y0, m->x1, m->y1);
}

static void printSubsst(Sst *r, size_t depth, size_t maxDepth) {
  if (r) {
    size_t i, j;

    printf("%llu. %d: ", depth, r->pl);
    printMove(r->move);
    printf("\n");
    printBoard(r->b, r->n);
    printf("\n");
    for (i = 0; i < r->nChild; ++i) {
      if (depth + 1 <= maxDepth) {
        for (j = 0; j <= depth; ++j) {
          printf("  ");
        }
        printSubsst(r->children[i], depth + 1, maxDepth);
      }
    }
  }
}

static void printSst(Sst *r, size_t maxDepth) {
  printf("\nSst:\n");
  printSubsst(r, 0, maxDepth);
}

static Move *initMove(size_t x0, size_t y0, size_t x1, size_t y1) {
  Move *m = malloc(sizeof(Move));
  m->x0 = x0;
  m->y0 = y0;
  m->x1 = x1;
  m->y1 = y1;
  setMiddle(m);
  return m;
}

static void insert(Sst *r, Move *m, char **b, int pl) {
  Sst **child = &(r->children[r->nChild++]);
  if (r->nChild >= r->sChild) {
    r->sChild *= 2;
    r->children = realloc(r->children, r->sChild * sizeof(Sst *));
  }
  /* printf("\ninsert: pl: %d\n", pl); */
  *child = sst(m, b, r->n, pl);
  (*child)->pl = pl;
}

static void freeSst(Sst *r) {
  size_t i;
  for (i = 0; i < r->nChild; ++i) {
    freeSst(r->children[i]);
  }
  free(r->children);
  free(r->move);
  for (i = 0; i < r->n; ++i) {
    free(r->b[i]);
  }
  free(r->b);
  free(r);
}

static char **initB(char **b, size_t n, char *colors) {
  size_t i, j;
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
  return b;
}

static char *serialize(char **b, int pl, size_t n) {
  size_t i, j = 0, k = 0;
  char *res = calloc((n * n + 2), sizeof(char));
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      res[k++] = b[i][j];
    }
  }
  res[k++] = (char) pl + '0';
  return res;
}

static size_t strToNum(char *str, size_t strLen) {
  double num = 0;
  size_t res;
  size_t i;
  size_t r = 3;
  for (i = 0; i < strLen; ++i) {
    size_t power = strLen - i - 1;
    double powerRes = pow((double) r, (double) power);
    int charVal;
    charVal = str[i] - '0' + 1;
    num = num + powerRes * charVal;
  }
  res = (size_t) num;
  return res;
}

static size_t h1(size_t key, size_t m) { return key % m; }

static size_t h2(size_t key, size_t hashLen) {
  size_t m2 = hashLen - 2;
  return 1 + (key % m2);
}

static size_t compHashIdx(size_t h1Val, size_t h2Val, size_t i,
                          size_t hashLen) {
  return (h1Val + (i * h2Val)) % hashLen;
}

static AddRes *add(char **hash, size_t hashLen, char *str, size_t strLen,
                   size_t *nFilled) {
  size_t key;
  size_t i = 0;
  int inserted = 0;
  size_t hashIdx = (size_t) -1;
  size_t h1Val;
  size_t h2Val;
  int exists = 0;
  AddRes *res = malloc(sizeof(AddRes));
  key = strToNum(str, strLen);
  h1Val = h1(key, hashLen);
  h2Val = h2(key, hashLen);
  while (inserted == 0 && exists == 0 && i < hashLen) {
    hashIdx = compHashIdx(h1Val, h2Val, i, hashLen);
    if (hash[hashIdx] == 0) {
      hash[hashIdx] = str;
      inserted = 1;
    } else if (strcmp(hash[hashIdx], str) == 0) {
      exists = 1;
    }
    ++i;
  }
  if (inserted == 1) {
    res->state = Added;
    res->index = hashIdx;
    ++*nFilled;
  } else if (exists == 1) {
    res->state = Exists;
  } else {
    res->state = Filled;
  }
  return res;
}

static void compSst(Sst *r, char *colors, int useHash, size_t *nInsert,
                    char **hash, size_t hashLen, double HASH_LOAD_FACTOR, size_t *nFilled,
                    int pl) {
  size_t i, j, q;
  int k, z, kk, zz;
  for (i = 0; i < r->n; ++i) {
    for (j = 0; j < r->n; ++j) {
      if (r->b[i][j] != colors[0]) {
        for (k = -1; k <= 1; ++k) {
          for (z = -1; z <= 1; ++z) {
            if (abs(k) != abs(z)) {
              if (canMove(r->b, r->n, colors, i, j, k, z)) {
                char **newB;
                Move *m = initMove(i, j, (size_t) ((int) i + 2 * k),
                                   (size_t) ((int) j + 2 * z));
                char *row;
                size_t rowLen = r->n * r->n + 1;
                AddRes *addRes;
                newB = copyB(r->b, r->n);
                moveNoMem(newB, colors, m);
                if (useHash) {
                  row = serialize(newB, pl, r->n);
                  addRes = add(hash, hashLen, row, rowLen, nFilled);
                  if (addRes->state == Filled) {
                    printf("\nHash is filled, could not add\n");
                    exit(EXIT_FAILURE);
                  }
                  if (addRes->state == Added) {
                    Sst *child = NULL;
                    int nextPlayer = (pl + 1) % 2;
                    int currentRow = i + 2 * k, currentColumn = j + 2 * z;
                    insert(r, m, newB, (pl + 1) % 2);
                    ++*nInsert;
//                    printf("\nnInsert: %lu\n", *nInsert);
                    child = r->children[r->nChild - 1];
                    compSst(child, colors, useHash,
                            nInsert, hash, hashLen, HASH_LOAD_FACTOR, nFilled,
                            nextPlayer);

                    for (kk = -1; kk <= 1; ++kk) {
                      for (zz = -1; zz <= 1; ++zz) {
                        if (abs(kk) != abs(zz)) {
                          if (canMove(child->b, r->n, colors, currentRow, currentColumn, kk, zz)) {
                            Move *move = initMove(currentRow, currentColumn,
                                                  (size_t) ((int) currentRow + 2 * kk),
                                                  (size_t) ((int) currentColumn + 2 * zz));
                            char **grandChildBoard = copyB(child->b, child->n);
                            moveNoMem(grandChildBoard, colors, move);
                            insert(child, move, grandChildBoard, child->pl);
                            ++*nInsert;
//                            printf("\nnInsert: %lu\n", *nInsert);
                            compSst(child->children[child->nChild - 1], colors, useHash, nInsert,
                                    hash, hashLen, HASH_LOAD_FACTOR, nFilled, child->pl);
                          }
                        }
                      }
                    }
                  } else {
                    free(row);
                    for (q = 0; q < r->n; ++q) {
                      free(newB[q]);
                    }
                    free(newB);
                    free(m);
                  }
                  free(addRes);
                } else {
                  Sst *child = NULL;
                  int nextPlayer = (pl + 1) % 2;
                  int currentRow = i + 2 * k, currentColumn = j + 2 * z;

                  insert(r, m, newB, nextPlayer);
                  ++*nInsert;
//                  printf("\nnInsert: %lu\n", *nInsert);
                  child = r->children[r->nChild - 1];
                  compSst(child, colors, useHash, nInsert,
                          hash, hashLen, HASH_LOAD_FACTOR, nFilled, nextPlayer);

                  for (kk = -1; kk <= 1; ++kk) {
                    for (zz = -1; zz <= 1; ++zz) {
                      if (abs(kk) != abs(zz)) {
                        if (canMove(child->b, r->n, colors, currentRow, currentColumn, kk, zz)) {
                          Move *move = initMove(currentRow, currentColumn,
                                                (size_t) ((int) currentRow + 2 * kk),
                                                (size_t) ((int) currentColumn + 2 * zz));
                          char **grandChildBoard = copyB(child->b, child->n);
                          moveNoMem(grandChildBoard, colors, move);
                          insert(child, move, grandChildBoard, child->pl);
                          ++*nInsert;
//                          printf("\nnInsert: %lu\n", *nInsert);
                          compSst(child->children[child->nChild - 1], colors, useHash, nInsert,
                                  hash, hashLen, HASH_LOAD_FACTOR, nFilled, child->pl);
                        }
                      }
                    }
                  }

                }
              }
            }
          }
        }
      }
    }
  }
}

static int checkPrime(size_t num) {
  size_t i;
  int isPrime = 1;
  if (num <= 3) {
    return 1;
  }
  for (i = 2; i * i <= num && isPrime == 1; ++i) {
    if (num % i == 0) {
      isPrime = 0;
    }
  }
  return isPrime;
}

static size_t compHashLen(size_t n, double lf) {
  size_t quotient = (size_t) (ceil((double) n / lf));
  size_t m;
  int isPrime = 0;
  m = quotient - 1;
  while (isPrime == 0) {
    ++m;
    isPrime = checkPrime(m);
  }
  return m;
}

static void freeHash(char **hash, size_t hashLen) {
  size_t i;
  for (i = 0; i < hashLen; ++i) {
    free(hash[i]);
  }
  free(hash);
}

static void testAlgo(char *colors) {
  int INITIAL_PLAYER_ID = 0;
  size_t BOARD_LENGTH = 5;
  int USE_HASH = TRUE;
  size_t HASH_ENTRY_COUNT = 10000000;
  double HASH_LOAD_FACTOR = 0.1;
  Move *INITIAL_MOVE = initMove(0, 2, 2, 2);
  int MAX_PRINT_DEPTH = INT_MAX;

  Sst *r;
  char **board = NULL;
  size_t nInsert = 0;
  size_t hashLen = compHashLen(HASH_ENTRY_COUNT, HASH_LOAD_FACTOR);
  size_t nFilled = 0;
  char **hash;

  board = initB(board, BOARD_LENGTH, colors);
  hash = calloc(hashLen, sizeof(char *));
  moveNoMem(board, colors, INITIAL_MOVE);
  printf("\nInitial board: \n");
  printBoard(board, BOARD_LENGTH);
  r = sst(INITIAL_MOVE, board, BOARD_LENGTH, INITIAL_PLAYER_ID);
  if (USE_HASH) {
    printf("\nUses cache\n");
  } else {
    printf("\nDoes not use cache\n");
  }
  compSst(r, colors, USE_HASH, &nInsert, hash, hashLen, HASH_LOAD_FACTOR, &nFilled, 0);
//  printSst(r, MAX_PRINT_DEPTH);
  printf("\nInsert count: %lu\n", nInsert);
  freeSst(r);
  freeHash(hash, hashLen);
  exit(EXIT_SUCCESS);
}

int main() {
  size_t n;
  char **b = NULL;
  size_t i;
  char input;
  char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
  int mode;
  int pl = 0;
  srand((unsigned) time(NULL));
  printf("Do you want to continue the previous game ('y': yes)? ");
  scanf(" %c", &input);
  if (input == 'y') {
    FILE *inf = fopen("skippity.txt", "r");
    if (!inf) {
      printf("\nPrevious game does not exist\n");
      exit(EXIT_SUCCESS);
    }
    char *buf = malloc(1024 * sizeof(char));
    size_t k;
    fgets(buf, 1024, inf);
    fclose(inf);
    printf("\nBuf: %s\n", buf);
    k = strlen(buf);
    n = (size_t) (sqrt((double) (k - 1)));
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

    b = initB(b, n, colors);
  }
  printf("\nBoard: \n");
  printBoard(b, n);
  printf("\nEnter game mode (0: human, 1: computer): ");
  scanf(" %d", &mode);
  if (mode == 0) {
    playWithHuman(b, n, colors, pl);
  } else {
//    playWithComputer(b, n, colors, pl);
  }
  for (i = 0; i < n; ++i) {
    free(b[i]);
  }
  free(b);
  return 0;
}
