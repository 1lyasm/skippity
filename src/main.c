#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define N_SKIPPER 5

typedef enum { Exists, Filled, Added } HashAddState;

typedef enum {
  MinimizeOpponentCaptures = 0,
  MaximizeCaptures = 1,
  Win = 2
} Strategy;

typedef struct {
  HashAddState state;
  size_t addedIndex;
} HashAddResult;

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
  double positionValue;
  struct Sst **children;
} Sst;

typedef struct {
  char lastBef;
  char lastMid;
  char lastAft;
} History;

static void printIntegerArray(int *arr, size_t n) {
  size_t i;
  for (i = 0; i < n; ++i) {
    printf("%d ", arr[i]);
  }
  printf("\n");
}

static void moveNoMem(char **b, char *colors, Move *p, int *counts0,
                      int *counts1, int pl) {
  char taken = b[p->mx][p->my];
  b[p->mx][p->my] = colors[0];
  b[p->x1][p->y1] = b[p->x0][p->y0];
  b[p->x0][p->y0] = colors[0];
  if (pl == 0) {
    ++counts0[taken - 'A'];
  } else {
    ++counts1[taken - 'A'];
  }
}

static void printBoard(char **b, size_t n) {
  size_t i, j;
  printf("\n    ");
  for (i = 0; i < n; ++i) {
    printf("%d ", (int)i);
    if (i < 10) {
      printf(" ");
    }
  }
  printf("\n");
  for (i = 0; i < n; ++i) {
    if (i < 10) {
      printf(" ");
    }
    printf("%d  ", (int)i);
    for (j = 0; j < n; ++j) {
      printf("%c ", b[i][j]);

      printf(" ");
    }
    printf("\n");
  }
}

static char **copyBoard(char **b, size_t n) {
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

static Sst *constructSst(Move *move, char **board, size_t boardLength,
                         int movingPlayer) {
  Sst *sst = malloc(sizeof(Sst));
  sst->move = move;
  sst->b = board;
  sst->n = boardLength;
  sst->pl = movingPlayer;
  sst->nChild = 0;
  sst->sChild = 16;
  sst->positionValue = -INT_MAX;
  sst->children = malloc(sst->sChild * sizeof(Sst *));
  return sst;
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

static void switchPlayer(int *pl, int *nUndo, int *nRedo, int *wantsRedo,
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

static void switchPlayerImmediately(int *pl, int *nUndo, int *nRedo,
                                    int *wantsRedo, int *hPos) {
  *pl = (*pl + 1) % 2;
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

static void computeScores(int *counts0, int *counts1, int *score0,
                          int *score1) {
  *score0 = findMin(counts0, N_SKIPPER);
  *score1 = findMin(counts1, N_SKIPPER);
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
  fclose(outf);
  free(buf);
}

static int inBoard(int x, int y, size_t n) {
  int n2 = (int)n;
  return x >= 0 && x < n2 && y >= 0 & y < n2;
}

static int canMove(char **b, size_t n, char *colors, size_t i, size_t j, int di,
                   int dj) {
  return inBoard((int)i + di, (int)j + dj, n) &&
         inBoard((int)i + 2 * di, (int)j + 2 * dj, n) &&
         b[(int)i + di][(int)j + dj] != colors[0] &&
         b[(int)i + 2 * di][(int)j + 2 * dj] == colors[0];
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
          switchPlayer(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
        }
      } else {
        wantsRedo = 0;
        if (nUndo == 0) {
          switchPlayerImmediately(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
        }
      }
    } else {
      printf("\nDo you want to save and exit ('y': yes)? ");
      scanf(" %c", &input);
      if (input == 'y') {
        save(b, n, pl);
        exit(EXIT_SUCCESS);
      }
      printf("\nPlayer %c, enter your move (x0, y0, x1, y1): ", pl + '0');
      scanf(" %lu %lu %lu %lu", &p.x0, &p.y0, &p.x1, &p.y1);
      setMiddle(&p);
      move(b, colors, &p, &h, pl, counts0, counts1);
      printBoard(b, n);
      computeScores(counts0, counts1, &score0, &score1);
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
          //          printf("\nCan undo: %d\n", canUndo);
        } else {
          undoes = 0;
        }
        if (!(undoes && nUndo == 0) || !canUndo) {
          switchPlayer(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
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

static Move *initMove(size_t x0, size_t y0, size_t x1, size_t y1) {
  Move *m = malloc(sizeof(Move));
  m->x0 = x0;
  m->y0 = y0;
  m->x1 = x1;
  m->y1 = y1;
  setMiddle(m);
  return m;
}

static Move *findAMove(char **board, size_t boardLength, char *colors) {
  Move *move = NULL;
  int found = 0;
  size_t i, j;
  for (i = 0; i < boardLength && !found; ++i) {
    for (j = 0; j < boardLength && !found; ++j) {
      if (board[i][j] != colors[0]) {
        if (canMove(board, boardLength, colors, i, j, 1, 0)) {
          move = initMove(i, j, i + 2, j);
          found = 1;
        } else if (canMove(board, boardLength, colors, i, j, 0, 1)) {
          move = initMove(i, j, i, j + 2);
          found = 1;
        } else if (canMove(board, boardLength, colors, i, j, -1, 0)) {
          move = initMove(i, j, i - 2, j);
          found = 1;
        } else if (canMove(board, boardLength, colors, i, j, 0, -1)) {
          move = initMove(i, j, i, j - 2);
          found = 1;
        }
      }
    }
  }
  return move;
}

static void printMove(Move *m) {
  printf("[ %lu, %lu -> %lu, %lu ]", m->x0, m->y0, m->x1, m->y1);
}

static void printSubsst(Sst *r, size_t depth, size_t maxDepth,
                        int printsBoard) {
  if (r) {
    size_t i, j;

    printf("%lu. %d: ", depth, r->pl);
    printMove(r->move);
    printf(", %lf\n", r->positionValue);
    if (printsBoard) {
      printBoard(r->b, r->n);
    }
    printf("\n");
    for (i = 0; i < r->nChild; ++i) {
      if (depth + 1 <= maxDepth) {
        for (j = 0; j <= depth; ++j) {
          printf("  ");
        }
        printSubsst(r->children[i], depth + 1, maxDepth, printsBoard);
      }
    }
  }
}

static void printSst(Sst *r, size_t maxDepth) {
  int PRINTS_BOARD = 0;
  printf("\nSst:\n");
  printSubsst(r, 0, maxDepth, PRINTS_BOARD);
}

static void insert(Sst *r, Move *m, char **b, int pl) {
  if (r->nChild + 1 >= r->sChild) {
    r->sChild *= 2;
    r->children = realloc(r->children, r->sChild * sizeof(Sst *));
  }
  r->children[r->nChild] = constructSst(m, b, r->n, pl);
  r->children[r->nChild]->pl = pl;
  ++(r->nChild);
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

static char *serializeBoard(char **board, size_t boardLength,
                            int movingPlayer) {
  size_t i, j = 0, k = 0;
  char *res = calloc((boardLength * boardLength + 2), sizeof(char));
  for (i = 0; i < boardLength; ++i) {
    for (j = 0; j < boardLength; ++j) {
      res[k++] = board[i][j];
    }
  }
  res[k++] = (char)movingPlayer + '0';
  return res;
}

static size_t strToNum(char *str, size_t strLen) {
  double num = 0;
  size_t res;
  size_t i;
  size_t r = 3;
  for (i = 0; i < strLen; ++i) {
    size_t power = strLen - i - 1;
    double powerRes = pow((double)r, (double)power);
    int charVal;
    charVal = str[i] - '0' + 1;
    num = num + powerRes * charVal;
  }
  res = (size_t)num;
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

static HashAddResult *addToHash(char **hash, size_t hashLen, char *str,
                                size_t strLen) {
  size_t key;
  size_t i = 0;
  int inserted = 0;
  size_t hashIdx = (size_t)-1;
  size_t h1Val;
  size_t h2Val;
  int exists = 0;
  HashAddResult *res = malloc(sizeof(HashAddResult));
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
    res->addedIndex = hashIdx;
  } else if (exists == 1) {
    res->state = Exists;
  } else {
    res->state = Filled;
  }
  return res;
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
  size_t quotient = (size_t)(ceil((double)n / lf));
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

static void branchFromPosition(Sst *root, char *colors, char **hashTable,
                               size_t hashLength, int depth, int maxSearchDepth,
                               Strategy strategy, int *counts0, int *counts1) {
  size_t row, column;
  int rowChange, columnChange;
  int isTerminalNode = 1;
  int *countsCopy0 = calloc(N_SKIPPER, sizeof(int));
  int *countsCopy1 = calloc(N_SKIPPER, sizeof(int));
  size_t j;
  for (row = 0; row < root->n; ++row) {
    for (column = 0; column < root->n; ++column) {
      if (root->b[row][column] != colors[0]) {
        for (rowChange = -1; rowChange <= 1; ++rowChange) {
          for (columnChange = -1; columnChange <= 1; ++columnChange) {
            if (abs(rowChange) != abs(columnChange)) {
              if (canMove(root->b, root->n, colors, row, column, rowChange,
                          columnChange)) {
                char **newBoard;
                size_t finalRow = row + 2 * (size_t)rowChange,
                       finalColumn = column + 2 * (size_t)columnChange;
                Move *move = initMove(row, column, finalRow, finalColumn);
                char *boardAsString;
                size_t boardStringLength = root->n * root->n + 1;
                HashAddResult *addResult;
                int nextPlayer = (root->pl + 1) % 2;
                for (j = 0; j < N_SKIPPER; ++j) {
                  countsCopy0[j] = counts0[j];
                  countsCopy1[j] = counts1[j];
                }
                newBoard = copyBoard(root->b, root->n);
                moveNoMem(newBoard, colors, move, countsCopy0, countsCopy1,
                          nextPlayer);
                boardAsString = serializeBoard(newBoard, root->n, nextPlayer);
                addResult = addToHash(hashTable, hashLength, boardAsString,
                                      boardStringLength);
                if (addResult->state == Added) {
                  Sst *childRoot = NULL;
                  int secondRowChange, secondColumnChange;
                  if (depth + 1 <= maxSearchDepth) {
                    isTerminalNode = 0;
                    insert(root, move, newBoard, nextPlayer);
                    childRoot = root->children[root->nChild - 1];
                    branchFromPosition(childRoot, colors, hashTable, hashLength,
                                       depth + 1, maxSearchDepth, strategy,
                                       countsCopy0, countsCopy1);
                    for (secondRowChange = -1; secondRowChange <= 1;
                         ++secondRowChange) {
                      for (secondColumnChange = -1; secondColumnChange <= 1;
                           ++secondColumnChange) {
                        if (abs(secondRowChange) != abs(secondColumnChange)) {
                          if (canMove(childRoot->b, childRoot->n, colors,
                                      finalRow, finalColumn, secondRowChange,
                                      secondColumnChange)) {
                            Move *continuationMove = initMove(
                                finalRow, finalColumn,
                                (size_t)((int)finalRow + 2 * secondRowChange),
                                (size_t)((int)finalColumn +
                                         2 * secondColumnChange));
                            char **grandChildBoard =
                                copyBoard(childRoot->b, childRoot->n);
                            moveNoMem(grandChildBoard, colors, continuationMove,
                                      countsCopy0, countsCopy1, childRoot->pl);
                            if (depth + 2 <= maxSearchDepth) {
                              size_t i;
                              childRoot->positionValue = -INT_MAX;
                              insert(childRoot, continuationMove,
                                     grandChildBoard, childRoot->pl);
                              branchFromPosition(
                                  childRoot->children[childRoot->nChild - 1],
                                  colors, hashTable, hashLength, depth + 2,
                                  maxSearchDepth, strategy, countsCopy0,
                                  countsCopy1);
                              for (i = 0; i < childRoot->nChild; ++i) {
                                if (childRoot->children[i]->positionValue >
                                    childRoot->positionValue) {
                                  childRoot->positionValue =
                                      childRoot->children[i]->positionValue;
                                }
                              }
                              for (i = 0; i < childRoot->nChild; ++i) {
                                if (childRoot->children[i]->pl == 0 &&
                                    childRoot->pl == 0) {
                                  if (childRoot->children[i]->positionValue <
                                      childRoot->positionValue) {
                                    childRoot->positionValue =
                                        childRoot->children[i]->positionValue;
                                  }
                                }
                              }

                            } else {
                              size_t i;
                              free(continuationMove);
                              for (i = 0; i < childRoot->n; ++i) {
                                free(grandChildBoard[i]);
                              }
                              free(grandChildBoard);
                            }
                          }
                        }
                      }
                    }
                  } else {
                    size_t i;
                    for (i = 0; i < root->n; ++i) {
                      free(newBoard[i]);
                    }
                    free(newBoard);
                    free(move);
                  }
                } else {
                  size_t i;
                  free(boardAsString);
                  for (i = 0; i < root->n; ++i) {
                    free(newBoard[i]);
                  }
                  free(newBoard);
                  free(move);
                }
                free(addResult);
              }
            }
          }
        }
      }
    }
  }
  if (isTerminalNode) {
    int score0, score1, scoreSum;
    int counts0Sum = 0, counts1Sum = 0;
    computeScores(counts0, counts1, &score0, &score1);
    scoreSum = score0 + score1;
    for (j = 0; j < N_SKIPPER; ++j) {
      counts0Sum += counts0[j];
      counts1Sum += counts1[j];
    }
    if (strategy == Win) {
      if (scoreSum > 0) {
        root->positionValue = (double)score1 / (score0 + score1);
      } else {
        root->positionValue = 0.5;
      }
    } else if (strategy == MaximizeCaptures) {
      root->positionValue = counts1Sum;
    } else if (strategy == MinimizeOpponentCaptures) {
      root->positionValue = -counts0Sum;
    }
  } else {
    size_t i;
    for (i = 0; i < root->nChild; ++i) {
      if (root->children[i]->positionValue > root->positionValue) {
        root->positionValue = root->children[i]->positionValue;
      }
    }
    for (i = 0; i < root->nChild; ++i) {
      if (root->children[i]->pl == 0 && root->pl == 0) {
        if (root->children[i]->positionValue < root->positionValue) {
          root->positionValue = root->children[i]->positionValue;
        }
      }
    }
  }
  free(countsCopy0);
  free(countsCopy1);
}

static Move *findBestMove(char **board, size_t boardLength, int movingPlayer,
                          char *colors, Strategy strategy, Move *opponentMove,
                          int *counts0, int *counts1) {
  size_t HASH_ENTRY_COUNT = (size_t)1e6;
  double HASH_LOAD_FACTOR = 0.1;

  size_t hashLength = compHashLen(HASH_ENTRY_COUNT, HASH_LOAD_FACTOR);
  char **hashTable = calloc(hashLength, sizeof(char *));

  char **boardCopy = copyBoard(board, boardLength);
  Sst *root = constructSst(opponentMove, boardCopy, boardLength,
                           (movingPlayer + 1) % 2);
  int maxSearchDepth;

  if (boardLength <= 6) {
    maxSearchDepth = 5;
  } else if (boardLength <= 10) {
    maxSearchDepth = 3;
  } else if (boardLength <= 20) {
    maxSearchDepth = 2;
  } else {
    maxSearchDepth = 1;
  }

  branchFromPosition(root, colors, hashTable, hashLength, 0, maxSearchDepth,
                     strategy, counts0, counts1);

  printSst(root, boardLength);

  freeSst(root);

  freeHash(hashTable, hashLength);

  return findAMove(board, boardLength, colors);
}

static void playWithComputer(char **b, size_t n, char *colors, int pl) {
  int ended = 0, nUndo = 0, nRedo = 0, hPos = 0, redoes, undoes, canUndo,
      wantsRedo = 1, score0, score1, i;
  Move p;
  char input;
  int *counts0 = calloc(N_SKIPPER, sizeof(int));
  int *counts1 = calloc(N_SKIPPER, sizeof(int));
  History h;
  Strategy strategy;
  printf("\nWhich strategy will be used ('0': minimize opponent captures, "
         "'1': maximize captures, '2': win)? ");
  scanf(" %d", &strategy);
  while (ended == 0) {
    if (pl == 0) {
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
          switchPlayer(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
        } else {
          wantsRedo = 0;
          if (nUndo == 0) {
            switchPlayerImmediately(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
          }
        }
      } else {
        printf("\nDo you want to save and exit ('y': yes)? ");
        scanf(" %c", &input);
        if (input == 'y') {
          save(b, n, pl);
          exit(EXIT_SUCCESS);
        }
        printf("\nPlayer %c, enter your move (x0, y0, x1, y1): ", pl + '0');
        scanf(" %lu %lu %lu %lu", &p.x0, &p.y0, &p.x1, &p.y1);
        setMiddle(&p);
        move(b, colors, &p, &h, pl, counts0, counts1);
        printBoard(b, n);
        computeScores(counts0, counts1, &score0, &score1);
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
          } else {
            undoes = 0;
          }
          if (!(undoes && nUndo == 0) || !canUndo) {
            switchPlayer(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
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
    } else {
      Move *opponentMove = initMove(p.x0, p.y0, p.x1, p.y1);
      Move *bestMove = findBestMove(b, n, pl, colors, strategy, opponentMove,
                                    counts0, counts1);
      printf("\nComputer played: ");
      printMove(bestMove);
      printf("\n");
      setMiddle(bestMove);
      moveNoMem(b, colors, bestMove, counts0, counts1, pl);
      printBoard(b, n);
      computeScores(counts0, counts1, &score0, &score1);
      printf("\nScore of human: %d, computer: %d\n", score0, score1);
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
      }
      switchPlayerImmediately(&pl, &nUndo, &nRedo, &wantsRedo, &hPos);
      free(bestMove);
    }
  }
  free(counts0);
  free(counts1);
}

int main() {
  size_t n;
  char **b = NULL;
  size_t i;
  char input;
  char colors[] = {'O', 'A', 'B', 'C', 'D', 'E'};
  int mode;
  int pl = 0;
  srand((unsigned)time(NULL));
  printf("\nDo you want to continue a previous game ('y': yes)? ");
  scanf(" %c", &input);
  if (input == 'y') {
    FILE *inf;
    char *buf = malloc(1024 * sizeof(char));
    size_t k;
    inf = fopen("skippity.txt", "r");
    if (!inf) {
      printf("\nPrevious game does not exist\n");
      exit(EXIT_SUCCESS);
    }
    fgets(buf, 1024, inf);
    fclose(inf);
    k = strlen(buf);
    n = (size_t)(sqrt((double)(k - 1)));
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
    playWithComputer(b, n, colors, pl);
  }
  for (i = 0; i < n; ++i) {
    free(b[i]);
  }
  free(b);
  return 0;
}
