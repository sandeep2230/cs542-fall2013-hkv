#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Datastructure to hold the 
 * routing cost matrix  specified in the input file 
 */
typedef struct {
  int numOfRouters;
  int **cost;
} OrigRoutingTable;

/* Datastructure to indicate links
 * linkid: Indicates a hop to hop link
 * linkType: Indicates a link type to classify the metric.
 * linkMetric: the value of linkType metric as an integer
 */ 
typedef struct {
  int linkid;
  int linkType;
  int linkMetric;
} Link;

typedef struct LinkList_ {
  Link *link;
  struct LinkList_ *next;
  struct LinkList_ *previous;
} LinkList;



/* Data structure to represent the 
 * link state packet
 * contains one or more direct links of the router.
 * this is supposed to be advertised to all the routers.
 * but in our implmentation we read it from the OrigRoutingTable.
 */
typedef struct {
  int lsAge;      /* Unused since we are constructing link state from
                     Original Routing Table */
  int lsId;       /* Unused : for the same reason as above */
  int lsSequence; /* Unused : for the same reason as above */
  int lsLength;   /* Unused : for the same reason as above */
  int numOfLinks;
  LinkList *linkFirst;
  LinkList *linkLast;
} LinkStatePacket;

/* Datastructure to represent the forwarding information
 * within a router. Cost represents the linkMetric.
 */
typedef struct {
  int dest;
  int nxtHop;
  int cost;
} Route;


/* Datastructure to represent the complete routing table
 * within a router. It contains the forwarding information 
 * as mentioned in the route.
 */
typedef struct RoutingTable {
  Route *rt;
  struct RoutingTable *next;
  struct RoutingTable *previous;
} RoutingTable;

/* This data structure represents the path from src to dst
 * in terms of hops between each router node in the path.
 */
typedef struct {
  int numOfHops;
  Route *rt;
} RoutePath;

OrigRoutingTable *ort; /* Allocated and filled once */
LinkStatePacket *lsp; /* Ideally link states are obtained through network advertisements
                         but we construct it from original routing table */ 
RoutingTable *rtTentative; /* Transient routing table during building of confirmed routing table. */
RoutingTable *rtConfirmed; /* Confirmed routing table. */
RoutingTable *rt;          /* Temporary routing table. */
RoutePath *path;

/* Function representing the high level menu */
static int get_input()
{
  char ch;
  printf ("1. Load File\n");
  printf ("2. Build Routing Table for Each Router\n");
  printf ("3. Out optimal path with minimum cost\n");
  printf ("0. Exit the program\n");
  ch = getchar();
  return ch - '0';  
}

/* Functio to read a line from the file
 * We first find out how much we have to allocate for the buffer, starting from 80 chars 
 * incrementing in steps of 80 if we feel short.
 * This function also returns the number of spaces between costs. Assumption is each cost
 * is delimited by a single space.
 */ 
static char *readLine(FILE *fd, int *lineLength_, int *numOfSpaces_) 
{
  const int buffer_size=80;
  char *line;
  char *line2;
  int lineIndex;
  int lineLength;
  int lineLength2;
  int numOfSpace=0;
  int err=0;

  /* Read a line
   * To accomodate lines of arbitrarily large numbe r of routers, 
   * assume tehre are 80 liines in the file, if it exceeds increase the line size by 80.
   */   
  lineLength = buffer_size;
  line = malloc(lineLength);
  memset(line, 0, lineLength);
  lineIndex=0;
  if (!feof(fd)) {
    numOfSpace = 0;
    while (!feof(fd)) {
        line[lineIndex] = fgetc(fd);
        if (line[lineIndex] == '\n') {
         /* read line completed */
         line[lineIndex] = '\0'; 
         break;
        }
        else if (lineIndex == (lineLength-1)) { 
          /* our buffer is small, lets increment it by buffer_size */
          lineLength2=lineLength+buffer_size; 
          line2 = realloc(line, lineLength2); 
          if (line2) {
            /* memset the trailing additionally allocated buffer to 0 */
            memset(line+lineLength, 0, buffer_size);
            line =  line2;
            lineLength = lineLength2;
          } else {
            err=1;
            break;
          }
        }

        /* Assume ech cost is delimited by a single space */
        if (line[lineIndex] == ' ') {
          numOfSpace++;
        }
        lineIndex++;
    }
  }

  if (err) {
    if (line) {
      free(line); 
    }
    return NULL;
  }
  *lineLength_ = lineLength;
  *numOfSpaces_ = numOfSpace;
  return line;
}

OrigRoutingTable *allocateOrigRoutingTable(int numOfSpace)
{
  OrigRoutingTable *ort;
  int i;
  int err=0;

  /* Allocate ort */
  ort = (OrigRoutingTable *) malloc(sizeof(OrigRoutingTable));
  if (!ort) {
      return NULL;
  }
  ort->numOfRouters = numOfSpace+1;
  ort->cost = malloc(sizeof(int *) * ort->numOfRouters);
  if (ort->cost) {
    memset(ort->cost, 0,sizeof(int *) * ort->numOfRouters); 
    for (i = 0; i < ort->numOfRouters; i++) {
      ort->cost[i] = malloc(sizeof(int) * ort->numOfRouters);
      if (!ort->cost[i]) {
        err = 1;
        break;
      }
      memset(ort->cost[i], 0,sizeof(int) * ort->numOfRouters); 
    }
  }
  else {
    err = 1;
  }

  if (err) {
    if (ort->cost) {
      for (i = 0; i < ort->numOfRouters; i++) {
        if (ort->cost[i]) {
          free(ort->cost[i]); 
        }
      }
      free(ort->cost); 
    }
    printf("Error in alloc routers for ort numOfRouters=%d",ort->numOfRouters);
    free(ort);
    return NULL;
  }
  return ort;  
}

int parseLine(char *line, int lineLength, int numOfRouters, int *cost)
{
   char *tempLine;
   char *tok = NULL;
   char delim[10]=" ";
   int i;

   tempLine = (char *) malloc(lineLength+1);

   memset(tempLine, 0, lineLength+1);

   strncpy(tempLine, line, lineLength);

   tok = strtok(tempLine, delim);
   if (tok) {
     for (i = 0; i < numOfRouters; i++) {
        cost[i] = atoi(tok);
        tok = strtok(NULL, delim);
        if (!tok) break;
     }
   }
   
   return 0;
}
/* Function to read the lines in the input file one by one
 * and construct the original routing table */
static OrigRoutingTable *loadOrigRoutingTable(char *file)
{
  FILE *fd;
  int err=0;
  int lineLength;
  char *line;
  int numOfRouters = 0;
  int numOfSpaces;
  OrigRoutingTable *ort = NULL;
  int i;

  /* Open file */
  fd = fopen(file, "r");
  if (!fd) {
    printf("Error in opening file: %s\n", file);
    return NULL;
  }

  
  /* Parse the fist line to know the size of the cost matrix. */
  line = readLine(fd, &lineLength, &numOfSpaces);
  if (!line) {
    printf("Error in reading line from %s\n", file);
    fclose(fd);
    return NULL;
  }
  printf("Num Of Spaces: %d\n", numOfSpaces); 

  /* Allocate Original Routing table */
  ort = allocateOrigRoutingTable(numOfSpaces);
  if (!ort) {
    printf("Error in allocating ORT");
    fclose(fd);
    return NULL;
  }

  parseLine(line, lineLength, ort->numOfRouters, ort->cost[0]);
  printf("Line %d: %s\n", 0, line); 
  for (i=1; i < ort->numOfRouters; i++) {
      line = readLine(fd, &lineLength, &numOfSpaces);
      if (!line) {
        printf("Error in reading line from %s\n", file);
        fclose(fd);
        return NULL;
      }
      printf("Line %d: %s\n", i, line); 
      parseLine(line, lineLength, ort->numOfRouters, ort->cost[i]);
  }

  return ort;
}

/* Function to add a link to LinkStatePacket */
static int addLinkToLSP(LinkStatePacket *lsp, Link *link)
{
  LinkList *links;
  links = (LinkList *) malloc(sizeof(LinkList));
  if (!links) {
    return -1;
  }

  links->link = link;
  links->next = NULL;
  links->previous = NULL;
  if (lsp->linkFirst == NULL) {
      lsp->linkFirst = lsp->linkLast = links;
  } else {
      lsp->linkLast->next = links;
      links->previous = lsp->linkLast;
      lsp->linkLast = links;
  }

  lsp->numOfLinks++;
}

/* Function to construct a link state packet */
static int constructLSP(OrigRoutingTable *ort, LinkStatePacket **lsp_)
{
  int i;
  int j;
  LinkStatePacket *lsp;

  if ((ort == NULL) || (lsp == NULL)) {
     printf("Wrong arguments ..\n");
     return -1;
  }

  lsp = (LinkStatePacket *) 
            malloc(sizeof(LinkStatePacket) * ort->numOfRouters);
  if (!lsp) {
     printf("Malloc failed..\n");
     return -1;
  }
  for (i = 0; i < ort->numOfRouters; i++) {
      memset(&lsp[i], 0, sizeof(LinkStatePacket));
      lsp[i].lsId = i;
      lsp[i].numOfLinks = 0;
      for (j = 0; j < ort->numOfRouters; j++) {
        if (ort->cost[i][j] != 0) {
         if (ort->cost[i][j] != -1) {
           Link *link;
           link = (Link *) malloc(sizeof(Link));
           link->linkid = i*ort->numOfRouters + j;
           link->linkType = 1; /* always same */
           link->linkMetric = ort->cost[i][j];
           addLinkToLSP(&lsp[i], link);
         }
        }
      }
  }
  *lsp_ = lsp;
  printf("Link added successfully\n");
  return 0;
}


static int printLSP(LinkStatePacket *lsp, int numOfRouters)
{
  LinkList *linkList;
  Link *link;
  int i, j; 
  for (i = 0; i < numOfRouters; i++) {
    printf("LSP %d\n", i);
    printf("\tlspId = %d\n", lsp[i].lsId);
    printf("\tnumOfLinks = %d\n", lsp[i].numOfLinks);
    linkList = lsp[i].linkFirst;
    for (j = 0; j < lsp[i].numOfLinks; j++) {
      if ( !linkList ) break;
      link = linkList->link;
      if (!link) {
        linkList = linkList->next;
        continue;
      }
      printf("\tLink:%d\n", j);
      printf("\t\tlinkid=%d\n", link->linkid);
      printf("\t\tlinkType=%d\n", link->linkType); /* always same */
      printf("\t\tlinkMetric=%d\n", link->linkMetric);
      linkList = linkList->next;
    }
  }
}

/* Function to buils routing table using available
 * LinkStatePackets.
 * This function uses Dijkstra's algorithm. 
 */
RoutingTable *buildRoutingTable(int routerId)
{
  return NULL;
}

void printRoutingTable(RoutingTable *rt)
{
  return;
}

RoutePath *computePath(int srcRouterId, int dstRouteerId)
{
  return NULL;
}

void printRoutePath(RoutePath *path)
{
  return;
}

int main(int argc, char **argv)
{
  int ch=1;
  int ret;
  while (ch != 0) {
    ch = get_input();
    printf("Your choice = %d\n", ch);
    switch (ch) {
      case 1:
      {
        char dataFile[25] = { 0 };
        printf("Please load original routing table data file\n");
        scanf("%s", dataFile); 
        ort = loadOrigRoutingTable(dataFile);
        if (!ort){ 
          printf("%s:%d error in constructing original routing table\n",
             __FUNCTION__, __LINE__);
          return -1;
        }
        printf("%s:%d ort=%p &lsp=%p\n",
             __FUNCTION__, __LINE__, ort, &lsp);

        ret = constructLSP(ort, &lsp);
        if (ret != 0) {
          printf("Link State Packets construction failure\n");
        } else {
          printf("Link State Packets construction success\n");
          printLSP(lsp, ort->numOfRouters);
        }

      }
      break;
      case 2:
      {
        int routerId;
        printf("Please select a router\n");
        scanf("%d", &routerId); 
        rt = buildRoutingTable(routerId);
        if (!rt){ 
          printf("%s:%d error in constructing routing table for routerID=%d\n",
             __FUNCTION__, __LINE__, routerId);
          return -1;
        }
        printf("The routing table for router %d is\n", routerId);
        printRoutingTable(rt);  
      }
      break;
      case 3:
      {
        int srcRouterId;
        int dstRouterId;
        printf("Please input the source and the destination router\n");
        scanf("%d", &srcRouterId); 
        scanf("%d", &dstRouterId); 
        path = computePath(srcRouterId, dstRouterId);
        if (!path){ 
          printf("%s:%d error in constructing path between %d and %d\n",
             __FUNCTION__, __LINE__, srcRouterId, dstRouterId);
          return -1;
        }
        printf("The path from %d to %d\n", srcRouterId, dstRouterId);
        printRoutePath(path);  
        break;
      }

    }
  }
}
