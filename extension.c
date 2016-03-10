/* Word ladder puzzle generator!
 * Chooses words at random and checks if a ladder can be built.  Then presents
 * the ladder with the middle words hidden and prompts the user to try and 
 * work out the solution.  Has an undo function to make things slightly easier.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#define PRINTWIDTH 5 /* words per line when printing ladders */
#define MINLEN 4 /* minimum length of a ladder.  2 or less will sometimes end 
 * badly, with a null pointer passed where it shouldn't be*/
#define WORDMIN 3 /* smallest length word allowed */

typedef enum mark {unvisited, visited} mark;
typedef enum warnings { warn_off, warn_on } warnings;

typedef struct node {
  char *word;
  mark visited;
  struct node *parent;
  struct node *qnext; /*used for the queue */
  struct node *next; /*used for the linked list */
} node;

typedef struct list {
  node *head;
  node *tail;
  int wlen; /*word length */
  int len; /*list length */
} list;

typedef struct queue {
  node *front;
  node *back;
} queue;

typedef struct ladder {
  node *start;
  node *end;
  int len;
  node **userladder; /* calloced array of pointers to nodes - the user's choices to fill the ladder with */
} ladder;

typedef struct buffer {
  char *str;
  short size;
} buffer;

buffer createBuffer(char **argv);
void checkArgs(int argc, char **argv);
void getFileInfo(char **argv, buffer *b);
void checkFile(FILE *file);
char *getInput(char *msg, buffer *b);
void createListfromFile(list *wlist, char **argv, buffer *b);
int checkWord( char *s, warnings w);

char *createString(int wlen, char *s);
node *createNode(char *s);
node *findNode_str(list wlist, char *s);
void findChildren(list wlist, node *parent, queue *q);
int findEd(node* n1, node* n2);
void freeList(list wlist);
void lowerCase( char *s);

void queueInit(queue *q);
void enQueue(node *n, queue *q);
node *deQueue(queue *q);
int  queueEmpty(queue *q);

/* extension functions */
node *findNode_num(list wlist, int num);
int  getListLen(list wlist);
int  getLadderLen(ladder wladder);
void clearMembers(list wlist);
void printLadder(ladder wladder);
int  checkForCommand(char *word, ladder *wladder, int *i);
int  addToLadder(char *word, ladder *wladder, int i, list wlist);
void initLadder(ladder *wladder, queue *q, list wlist);
int  checkDigit(char *s);

int main(int argc, char **argv)
{
  list wlist = { NULL, NULL, 0, 0 };  
  ladder wladder = { NULL, NULL, 0, NULL };
  queue q;
  buffer b;
  char *word;
  int i, err = 0;

  srand(time(NULL));  
  checkArgs(argc,argv);
  b = createBuffer(argv);
  getFileInfo(argv, &b);
  wlist.wlen = atoi(argv[2]);
  if (wlist.wlen > b.size - 1) {
    fprintf(stderr,"No words of this length in your dictionary.\n");
    exit(EXIT_FAILURE);
  }
  createListfromFile(&wlist, argv, &b);
  initLadder(&wladder, &q, wlist);
  
  for (i = 1; i < wladder.len - 1; i++) {
    printLadder(wladder); 
    do {
      err = 0;
      word = getInput("Enter next word, or \"UNDO\" to undo : ", &b);
      if (!checkForCommand(word, &wladder, &i)) {
        err = addToLadder(word, &wladder, i, wlist);
      }
    }
    while (err == 1);
  }
  
  printLadder(wladder); 
  if (findEd(wladder.userladder[i],wladder.userladder[i-1]) == 1) {
    fprintf(stderr,"You win!\n");
  }
  else {
    fprintf(stderr,"You lose...\n");
  }

  freeList(wlist);
  free(b.str);
  free(wladder.userladder);
  return 0;
}

void initLadder(ladder *wladder, queue *q, list wlist)
{
  
  do {
    wladder->start = findNode_num(wlist, rand() % wlist.len); 
    wladder->end = findNode_num(wlist, rand() % wlist.len);
    clearMembers(wlist);
    queueInit(q);
    enQueue(wladder->start, q);
    while ( wladder->end->parent == NULL && !queueEmpty(q) ) {
      findChildren(wlist,deQueue(q), q);
    }
    wladder->len = getLadderLen(*wladder);
  }
  while (wladder->end->parent == NULL || wladder->len < MINLEN);
  
  wladder->userladder = (node **)calloc(wladder->len,sizeof(node *));
  wladder->userladder[0] = wladder->start;
  wladder->userladder[wladder->len - 1] = wladder->end;
}

int checkForCommand(char *word, ladder *wladder, int *i) 
{
  if (strcmp(word,"UNDO") == 0) {
    if (*i > 1) {
      wladder->userladder[(*i)-1] = NULL;
      (*i)-=2;
    }
    else {
      fprintf(stdout,"Nothing to undo.\n");
      (*i)--;
    }
    return 1;
  }
  return 0;
}

int addToLadder(char *word, ladder *wladder, int i, list wlist)
{
  int err = 0;
  char c;
  
  if ((int)strlen(word) > wlist.wlen) {
    err = 1;
    fprintf(stdout,"That word is too long!\n");
    do {
      c = getchar();
    }
    while(c != '\n');
  }
  else if ((wladder->userladder[i] = findNode_str(wlist,word)) == NULL) {
    err = 1;
    if ((int)strlen(word) < wlist.wlen) {
      fprintf(stdout,"That word is too short!\n");
    }
    else {
      fprintf(stdout,"%s is not in your dictionary!", word);
      fprintf(stdout," Please try again\n");
    }
  }
  else if (findEd(wladder->userladder[i], wladder->userladder[i-1]) != 1) {
    err = 1;
    fprintf(stdout,"That is not a valid move!\n");
    wladder->userladder[i] = NULL;
  }
  free(word);
  return err;
}

void printLadder(ladder wladder)
{
  int i, j;
  
  printf("\n");
  for (i = 0; i < wladder.len; i++) {
    if (wladder.userladder[i] != NULL) {
      printf("%s",wladder.userladder[i]->word);
    }
    else {
      for (j = 0; j < (int)strlen(wladder.start->word); j++) {
        putchar('_');
      }
    }
    printf("\n");
  }
  printf("\n");
}

node *findNode_num(list wlist, int num)
{
  int i;
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  for (i = 0; (n != NULL && i != num); i++) {
    n = n->next;
  }
  if (n == NULL) {
  fprintf(stderr,"ERROR: number larger than list...\n");
  exit(EXIT_FAILURE);
  }
  else {
    return n;
  }
}

int getListLen(list wlist)
{
  int cnt = 0;
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  while (n != NULL) {
    cnt++;
    n = n->next;
  }
  return cnt;
}

int getLadderLen(ladder wladder)
{
  int cnt = 0;
  node *n = wladder.end; /* this assignment is just to avoid pointer chasing a  * var named end */ 
  
  while (n != NULL) {
    cnt++;
    n = n->parent;
  }
  return cnt;
}

void clearMembers(list wlist)
{
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  while (n != NULL) {
    n->qnext = NULL;
    n->parent = NULL;
    n->visited = unvisited;
    n = n->next;
  }
}

char *getInput(char *msg, buffer *b)
{
/* uses strcspn from string.h to remove the \n. */
  printf(msg);
  if (fgets(b->str,b->size + 1,stdin) != NULL) {
    b->str[strcspn(b->str,"\n")] = '\0';
    return createString(b->size, b->str);
  }
  else {
    printf("\nEOF used to exit program.\n");
    exit(EXIT_SUCCESS);
  }
}

void checkArgs(int argc, char **argv)
{
  if ((argc != 3) 
  ||  (argv[1] == NULL) 
  ||  (!checkDigit(argv[2])) ) {
    fprintf(stderr,"ERROR: Incorrect usage:\n");
    fprintf(stderr,"- Argument 1 must be a dictionary file.\n"); 
    fprintf(stderr,"- Argument 2 must be a 1-digit number >= %d, ", WORDMIN); 
    fprintf(stderr,"which determines the number of letters per word.\n");     
    fprintf(stderr,"- Both arguments are required.\n"); 
    exit(EXIT_FAILURE);    
  }
}

int checkDigit(char *s)
{
  int i;

  for (i = 0; s[i]; i++)  {
    if (!isdigit(s[i])) {
      return 0;
    }
  }
  if (atoi(s) < WORDMIN) {
    return 0;
  }
  return 1;
}

void checkFile(FILE *file)
{
/* This is called every time the file is reopened in case another process 
 * has messed with it since the last time. */
  if (file == NULL) {
    fprintf(stderr, "ERROR: failed to open file - ");
    fprintf(stderr,"check name and directory.\n");
    exit(EXIT_FAILURE);
  }
}

void getFileInfo(char **argv, buffer *b)
{
  FILE *file = fopen(argv[1], "r");
  int cnt = 0;
  
  checkFile(file);
  while ( fgets(b->str, b->size + 1, file) != NULL) {
    b->str[strcspn(b->str,"\n")] = '\0'; /* removes the newline */  
    if (checkWord(b->str,warn_on) && strlen(b->str) != 0) {
      cnt++;
    }
  }
  fclose(file);
  printf("%d words read\n",cnt);
}

void createListfromFile(list *wlist, char **argv, buffer *b)
{
  /* This is the only function which needs to modify struct list, so it 
   * is passed a pointer not a copy */
  FILE *file = fopen(argv[1], "r"); 
  checkFile(file);
  
  while ( fgets(b->str, b->size + 1, file) != NULL) {
    b->str[strcspn(b->str,"\n")] = '\0'; /* removes the newline */
    if ( (int)strlen(b->str) == wlist->wlen
    && checkWord(b->str,warn_off)) {
      lowerCase(b->str);
      if (wlist->head == NULL) {
        wlist->head = wlist->tail 
          = createNode(createString(wlist->wlen,b->str));
      }
      else {
        wlist->tail->next 
          = createNode(createString(wlist->wlen,b->str));
        wlist->tail = wlist->tail->next;
      }
    }
  }
  wlist->len = getListLen(*wlist);
  fclose(file);
}

int checkWord( char *s, warnings w)
{
  int i;

  for (i = 0; s[i]; i++)  {
    if (!isalpha(s[i])) {
      if (w == warn_on) {
        fprintf(stderr,"WARNING: %s was read from ", s);
        fprintf(stderr,"dictionary but discarded.\n");
      }
      return 0;
    }
  }
  return 1;
}

buffer createBuffer(char **argv)
/* bases the buffer size on the longest word in the dictionary file */
{
  FILE *file = fopen(argv[1], "r");
  int cnt = 0, maxcnt = 0;
  char c;
  buffer b;
  
  checkFile(file);
  while((c = fgetc(file)) != EOF) {
    cnt++;
    if (c == '\n') {
      if (cnt > maxcnt) {
        maxcnt = cnt;
      }
      cnt = 0;
    }
  }
  fclose(file);
  b.size = maxcnt;
  b.str = (char *)malloc(sizeof(char) * b.size);
  if (b.str == NULL) {
    fprintf(stderr,"ERROR: buffer malloc failed\n");
    exit(EXIT_FAILURE);
  }
  return b;
}

void findChildren(list wlist, node *parent, queue *q)
{  
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  parent->visited = visited;
  while (n != NULL) {
    if (n->visited != visited && findEd(n,parent) == 1 ) {
      n->parent = parent;
      n->visited = visited;
      enQueue(n,q);
    }
    n = n->next;
  }
}

int findEd(node* n1, node* n2)
/* ed: edit distance */
{
  int i, ed;
  
  for(i = 0, ed = 0; n1->word[i]; i++) {
    if (n1->word[i] != n2->word[i]) {
      ed++;
    }
  }
  return ed;
}

node *findNode_str(list wlist, char *s)
{
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  while (n != NULL) {
    if (strcmp(n->word,s) == 0) {
      return n;
    }
    n = n->next;
  }
  return NULL;
}

node *createNode(char *s)
{
  node *p;

  p = (node *)malloc(sizeof(node));
  if (p == NULL) {
    fprintf(stderr,"ERROR: node malloc failed\n");
    exit(EXIT_FAILURE);
  }
  p->word = s;
  p->visited = unvisited;
  p->qnext = NULL;
  p->next = NULL;
  p->parent = NULL;
  
  return p;
}

char* createString(int wlen, char *s)
{
  char *str = (char *)malloc(sizeof(char) * wlen + 1);
  if (str == NULL) {
    fprintf(stderr,"ERROR: string malloc failed\n");
    exit(EXIT_FAILURE);
  }
  strcpy(str, s);
  str[wlen] = '\0';

  return str;
}

void lowerCase( char *s)
{
  int i;

  for (i = 0; s[i]; i++)  {
    s[i] = tolower(s[i]);
  }
}

void freeList (list wlist)
{
  node *temp;
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  while (n != NULL) {
    temp = n;
    n = n->next;
    free(temp->word);
    free(temp);
  }
}

void queueInit(queue *q)
{
  q->front = q->back = NULL;
}

node *deQueue(queue *q)
{
  node *n = q->front;

  if (n != NULL) {
    q->front = q->front->qnext;
    n->qnext = NULL; /* remove stray queue pointers */
    if (q->front == NULL) {
      q->back = NULL; /* properly empties the queue */
    }
  }
  else {
    fprintf(stderr,"ERROR: attempted to deQueue() empty queue\n");
    exit(EXIT_FAILURE);
  }
  return n;
}

void enQueue(node *n, queue *q)
{
  if (q->back != NULL) {
    q->back->qnext = n;
    q->back = n;
  }
  else {
    q->back = q->front = n;
  }
}

int queueEmpty(queue *q)
{
  if(q->front == NULL) {
    return 1;
  }
  else {
    return 0;
  }
}