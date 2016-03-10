/* Word ladder generator!
 * given 2 word inputs of the same length, attempts to build a ladder between
 * them by changing one letter at a time.
 * Builds a linked list of correct-length words from the dictionary file in
 * argv[1], which is then overlaid with a linked list queue implementation
 * which uses the same nodes.
 * This enables a breadth-first search to find the shortest path by placing
 * each child node in the queue.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#define PRINTWIDTH 5 /*words per line when printing ladders */

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
} list;

typedef struct queue {
  node *front;
  node *back;
} queue;

typedef struct ladder {
  node *start;
  node *end;
} ladder;

typedef struct buffer {
  char *str;
  short size; /* max buffer size, must include EOS */
} buffer;

buffer createBuffer(char **argv);
void checkArgs(int argc, char **argv);
void getFileInfo(char **argv, buffer *b);
void checkFile(FILE *file);
char *getInput(char *msg, buffer *b);
void checkInput(char *sourceword, char *targetword);
void createListfromFile(list *wlist, char **argv, buffer *b);
void lowerCase(char *s);
int  checkWord(char *s, warnings w);

char *createString(int wlen, char *s);
node *createNode(char *s);
node *findNode(list wlist, char *s);
void findChildren(list wlist, node *parent, queue *q);
int  findEd(node* n1, node* n2);
void printLadder(node *end);
void printResults(ladder wladder);
void freeList(list wlist);

void queueInit(queue *q);
void enQueue(node *n, queue *q);
node *deQueue(queue *q);
int  queueEmpty(queue *q);

int main(int argc, char **argv)
{
  list wlist = { NULL, NULL, 0 };  
  ladder wladder = { NULL, NULL };
  queue q;
  char *sourceword, *targetword;
  buffer b;
  
  checkArgs(argc,argv);
  b = createBuffer(argv);
  getFileInfo(argv, &b);
  sourceword = getInput("Source word : ",&b);
  targetword = getInput("Target word : ",&b);
  checkInput(sourceword,targetword);
  wlist.wlen = strlen(sourceword);
  
  createListfromFile(&wlist, argv, &b);
  wladder.start = findNode(wlist,sourceword); 
  wladder.end = findNode(wlist,targetword);
  queueInit(&q);
  enQueue(wladder.start, &q);
  
  while (wladder.end->parent == NULL && !queueEmpty(&q) ) {
    findChildren(wlist,deQueue(&q), &q);
  }
  printResults(wladder);

  freeList(wlist);
  free(b.str);
  free(sourceword);
  free(targetword);
  return 0;
}

char *getInput(char *msg, buffer *b)
{
/* uses strcspn from string.h to check the num of chars before a \n, 
 * and also to remove the \n. */
  printf(msg);
  if (fgets(b->str,b->size + 1,stdin) != NULL) {
    if ((int)strcspn(b->str,"\n") > b->size - 1){
      fprintf(stderr,"ERROR: There are no words of this length ");
      fprintf(stderr,"in the dictionary file.\n");
      exit(EXIT_FAILURE);
    }
    b->str[strcspn(b->str,"\n")] = '\0';
    lowerCase(b->str);
    return createString(b->size, b->str);
  }
  else {
    printf("\nEOF used to exit program.\n");
    exit(EXIT_SUCCESS);
  }
}

void checkArgs(int argc, char **argv)
{
  if ( (argc != 2) || (argv[1] == NULL) )  {
    fprintf(stderr,"ERROR: Incorrect usage:\n");
    fprintf(stderr,"- Argument 1 must be a dictionary file.\n"); 
    fprintf(stderr,"- Only 1 argument is required.\n"); 
    exit(EXIT_FAILURE);    
  }
}

void checkInput(char *sourceword, char *targetword)
{
  if (strlen(sourceword) != strlen(targetword)) {
    fprintf(stderr,"ERROR: source and target words ");
    fprintf(stderr,"must be of equal length\n\n");
    exit(EXIT_FAILURE);
  }
  if (strcmp(sourceword,targetword) == 0) {
    fprintf(stderr,"ERROR: two different words required!\n\n");
    exit(EXIT_FAILURE);
  }
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
    if ((int)strlen(b->str) == wlist->wlen 
    &&  checkWord(b->str,warn_off)) {
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

node *findNode(list wlist, char *s)
{
  node *n = wlist.head; /* this assignment is just to avoid pointer chasing a  
  * var named head, which could be misleading */ 
  
  while (n != NULL) {
    if (strcmp(n->word,s) == 0) {
      return n;
    }
    n = n->next;
  }
  fprintf(stderr,"ERROR: %s not found in list\n", s);
  exit(EXIT_FAILURE);
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

void printLadder (node *end)
{
  static int cnt = 0;
  node *n = end; /* this assignment is just to avoid pointer chasing a var 
  * named end, while still showing that the function hould be passed the
  * ladder's end. */ 
   
  if (n == NULL) {
    return; /* base case */
  }
  /* has to be recursive in order to print the right way round */
  printLadder(n->parent);
  if (n->parent != NULL) {
    printf(" -> ");
  }
  if (cnt % PRINTWIDTH == 0) {
    printf("\n");
  }
  printf("%s",n->word);
  cnt++;
}

void printResults(ladder wladder)
{
  if (wladder.end->parent != NULL) {
    printLadder(wladder.end);
  }
  else {
    printf("\nNo ladder possible between these words!");
  }
  printf("\n\n");
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