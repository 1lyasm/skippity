#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_SKIPPER 5

typedef struct {
  int nVal;
  int sVal;
  char **vals;
} Cache;

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
  char **b;
  size_t n;
  size_t pl;
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
    printf("%d ", (int)i);
  }
  printf("\n");
  for (i = 0; i < n; ++i) {
    printf("%d ", (int)i);
    for (j = 0; j < n; ++j) {
      printf("%c ", b[i][j]);
    }
    printf("\n");
  }
}

static char **copyB(char **b, size_t n) {
  int i, j;
  char **newB = malloc(n * sizeof(char *));
  for (i = 0; i < n; ++i) {
    newB[i] = malloc(n * sizeof(char));
    for (j = 0; j < n; ++j) {
      newB[i][j] = b[i][j];
    }
  }
  return newB;
}

static Sst *sst(Move *m, char **b, int n, char *colors, int pl) {
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

static void move(char **b, char *colors, Move *p, History *h, int pl, int *counts0,
          int *counts1) {
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

static void switchP(int *pl, int *nUndo, int *nRedo, int *wantsRedo, int *hPos) {
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

static void compScore(int pl, int *counts0, int *counts1, int *score0, int *score1) {
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

static int canMove(char **b, size_t n, char *colors, int i, int j, int di, int dj) {
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
  printf("[(%d, %d), (%d, %d), (%d, %d)]", m->x0, m->y0, m->mx, m->my, m->x1,
         m->y1);
}

static void printSubsst(Sst *r, int indent) {
  if (r) {
    int i, j;
    printMove(r->move);
    printf(" ( %p )\n", r);
    for (i = 0; i < r->nChild; ++i) {
      for (j = 0; j < indent; ++j) {
        printf("\t");
      }
      printSubsst(r->children[i], indent + 1);
    }
  }
}

static void printSst(Sst *r) {
  printf("\nSst:\n");
  printSubsst(r, 1);
}

static Move *initMove(int x0, int y0, int x1, int y1) {
  Move *m = malloc(sizeof(Move));
  m->x0 = x0;
  m->y0 = y0;
  m->x1 = x1;
  m->y1 = y1;
  setMiddle(m);
  return m;
}

static void insert(Sst *r, Move *m, char **b, int pl, char *colors) {
  Sst **child = &(r->children[r->nChild++]);
  if (r->nChild >= r->sChild) {
    r->sChild *= 2;
    r->children = realloc(r->children, r->sChild * sizeof(Sst *));
  }
  *child = sst(m, b, r->n, colors, pl);
  (*child)->pl = pl;
}

static void freeSst(Sst *r) {
  int i;
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
  int i, j;
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

static char *serialize(char **b, int pl, int n) {
  int i, j = 0, k = 0;
  char *res = calloc((n * n + 2), sizeof(char));
  for (i = 0; i < n; ++i) {
    for (j = 0; j < n; ++j) {
      res[k++] = b[i][j];
    }
  }
  res[k++] = pl + '0';
  /* printf("\nserialize: res: %s\n", res); */
  /* printf("\nlength of res: %lu\n", strlen(res)); */
  return res;
}

static void addToCache(Cache *cache, int n, int pl, char *row) {
  int i;
  if (cache->nVal >= cache->sVal) {
    cache->sVal *= 2;
    cache->vals = realloc(cache->vals, cache->sVal * sizeof(char *));
  }
  cache->vals[cache->nVal] = row;
  ++(cache->nVal);
}

static int compCharArr(char *arr1, char *arr2, int n) {
  int i;
  int eq = 1;
  for (i = 0; i < n && eq; ++i) {
    if (arr1[i] != arr2[i]) {
      eq = 0;
    }
  }
  return eq;
}

static int inCache(Cache *cache, char **b, int n, int pl, char *row) {
  int isIn = 0, i;
  int rowLen = n * n + 1;
  printf("\ncache->nVal: %d\n", cache->nVal);
  for (i = 0; i < cache->nVal && !isIn; ++i) {
    if (compCharArr(row, cache->vals[i], rowLen)) {
      isIn = 1;
    }
  }
  return isIn;
}

static void freeCache(Cache *cache) {
  int i;
  for (i = 0; i < cache->nVal; ++i) {
    free(cache->vals[i]);
  }
  free(cache->vals);
  free(cache);
}

static size_t strToNum(char *str, int strLen) {
  double num = 0;
  size_t res;
  int i;
  int r = 3;
  for (i = 0; i < strLen; ++i) {
    int power = strLen - i - 1;
    double powerRes = pow(r, power);
    int charVal;
    charVal = str[i] - '0' + 1;
    num = num + powerRes * charVal;
  }
  res = (size_t)num;
  return num;
}

static size_t h1(size_t key, size_t m) { return key % m; }

static size_t h2(size_t key, size_t hashLen) {
  size_t m2 = hashLen - 2;
  return 1 + (key % m2);
}

static int compHashIdx(int h1Val, int h2Val, int i, int hashLen) {
  return ((size_t)h1Val + (size_t)(i * h2Val)) % (size_t)hashLen;
}

static int add(char **hash, size_t hashLen, double loadF, char *str, int strLen,
        int *nFilled) {
  size_t key;
  int i = 0, j;
  int inserted = 0;
  size_t hashIdx;
  size_t h1Val;
  size_t h2Val;
  int exists = 0;
  key = strToNum(str, strLen);
  h1Val = h1(key, hashLen);
  h2Val = h2(key, hashLen);
  while (inserted == 0 && exists == 0 && i < hashLen) {
    hashIdx = compHashIdx(h1Val, h2Val, i, hashLen);
    if (hash[hashIdx] == 0) {
      hash[hashIdx] = str;
      inserted = 1;
    } else if (strcmp(hash[hashIdx], str) == 0) {
      /* printf("\nadd: exists\n"); */
      exists = 1;
    }
    ++i;
  }
  if (inserted == 1) {
    ++*nFilled;
  } else if (exists == 1) {
    hashIdx = -1;
  } else {
    hashIdx = -2;
  }
  return hashIdx;
}

void compSst(Sst *r, char *colors, int useCache, int *nInsert, char **hash,
             int hashLen, double loadF, int *nFilled) {
  int i, j, k, z, q;
  int pl = 1;
  for (i = 0; i < r->n; ++i) {
    for (j = 0; j < r->n; ++j) {
      if (r->b[i][j] != colors[0]) {
        for (k = -1; k <= 1; ++k) {
          for (z = -1; z <= 1; ++z) {
            if (abs(k) != abs(z)) {
              if (canMove(r->b, r->n, colors, i, j, k, z)) {
                char **newB;
                Move *m = initMove(i, j, i + 2 * k, j + 2 * z);
                char *row;
                int rowLen = r->n * r->n + 1;
                int addedIdx = 0;
                newB = copyB(r->b, r->n);
                moveNoMem(newB, colors, m);
                if (useCache) {
                  row = serialize(newB, pl, r->n);
                  addedIdx = add(hash, hashLen, loadF, row, rowLen, nFilled);
                  if (addedIdx == -2) {
                    printf("\nHash is filled, could not add\n");
                    exit(EXIT_FAILURE);
                  }
                  if (addedIdx >= 0) {
                    insert(r, m, newB, pl, colors);
                    ++*nInsert;
                    printf("\nnInsert: %d\n", *nInsert);
                    if (rand() % 10 == 0) {
                      compSst(r->children[r->nChild - 1], colors, useCache,
                              nInsert, hash, hashLen, loadF, nFilled);
                    }
                  } else {
                    free(row);
                    for (q = 0; q < r->n; ++q) {
                      free(newB[q]);
                    }
                    free(newB);
                    free(m);
                  }
                } else {
                  insert(r, m, newB, pl, colors);
                  ++*nInsert;
                  printf("\nnInsert: %d\n", *nInsert);
                  compSst(r->children[r->nChild - 1], colors, useCache, nInsert,
                          hash, hashLen, loadF, nFilled);
                }
              }
            }
          }
        }
      }
    }
  }
}

static int checkPrime(int num) {
  int i;
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
  size_t quotient = (size_t)ceil(n / lf);
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
  int i;
  for (i = 0; i < hashLen; ++i) {
    free(hash[i]);
  }
  free(hash);
}

static void testAlgo(char *colors) {
  int pl = 0, i;
  Sst *r;
  char **b = NULL;
  size_t n = 6;
  int nInsert = 0;
  int usesCache = 1;
  size_t nEntry = 1000000;
  double loadF = 0.1;
  size_t hashLen = compHashLen(nEntry, loadF);
  Move *m = initMove(0, 2, 2, 2);
  int nFilled = 0;
  char **hash;
  printf("\nhashLen: %lu\n", hashLen);
  b = initB(b, n, colors);
  hash = calloc(hashLen, sizeof(char *));
  moveNoMem(b, colors, m);
  printf("\nInitial board: \n");
  printBoard(b, n);
  r = sst(m, b, n, colors, pl);
  if (usesCache) {
    printf("\nUses cache\n");
  } else {
    printf("\nDoes not use cache\n");
  }
  compSst(r, colors, usesCache, &nInsert, hash, hashLen, loadF, &nFilled);
  printf("\nInsert count: %d\n", nInsert);
  freeSst(r);
  freeHash(hash, hashLen);
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
  testAlgo(colors);
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

    b = initB(b, n, colors);
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
