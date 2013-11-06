#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEBUG(args...)  
//#define DEBUG printf
#define LOG   printf
#define ERR   printf
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
  Link **link;
} LinkStatePacket;

/* Datastructure to represent the forwarding information
 * within a router. Cost represents the linkMetric.
 */
typedef struct {
  int nxtHop;
  int cost;
} Route;


/* Datastructure to represent the complete routing table
 * within a router. It contains the forwarding information 
 * as mentioned in the route.
 */
typedef struct RoutingTable {
  Route **rt;
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
RoutePath *path;

/* Function representing the high level menu */
static int get_input()
{
  int ch;
  LOG ("1. Load File\n");
  LOG ("2. Build Routing Table for Each Router\n");
  LOG ("3. Out optimal path with minimum cost\n");
  LOG ("0. Exit the program\n");
  scanf("%d", &ch);
  return ch;  
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
    ERR("Error in alloc routers for ort numOfRouters=%d",ort->numOfRouters);
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
   int i=0;

   tempLine = (char *) malloc(lineLength+1);

   memset(tempLine, 0, lineLength+1);

   strcpy(tempLine, line);
   tok = strtok(tempLine, delim);
   while (tok) {
     cost[i] = atoi(tok);
     tok = strtok(NULL, delim);
     i++;
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
  int i, j;

  /* Open file */
  LOG("Open File %s\n", file);
  fd = fopen(file, "r");
  if (!fd) {
    ERR("Error in opening file: %s\n", file);
    return NULL;
  }

  
  /* Parse the fist line to know the size of the cost matrix. */
  line = readLine(fd, &lineLength, &numOfSpaces);
  if (!line) {
    ERR("Error in reading line from %s\n", file);
    fclose(fd);
    return NULL;
  }
  DEBUG("Num Of Spaces: %d\n", numOfSpaces); 

  /* Allocate Original Routing table */
  ort = allocateOrigRoutingTable(numOfSpaces);
  if (!ort) {
    ERR("Error in allocating ORT");
    fclose(fd);
    return NULL;
  }

  LOG("Original Routing Table is As Follows\n");
  parseLine(line, lineLength, ort->numOfRouters, ort->cost[0]);
  for(j = 0; j < ort->numOfRouters; j++)
          LOG("%2d  ", ort->cost[0][j]);
  LOG("\n");
  for (i=1; i < ort->numOfRouters; i++) {
      line = readLine(fd, &lineLength, &numOfSpaces);
      if (!line) {
        DEBUG("Error in reading line from %s\n", file);
        fclose(fd);
        return NULL;
      }

      parseLine(line, lineLength, ort->numOfRouters, ort->cost[i]);

      for(j = 0; j < ort->numOfRouters; j++)
          LOG("%2d  ", ort->cost[i][j]);
      LOG("\n");
  }

  return ort;
}

/* Function to construct a link state packet */
static int constructLSP(OrigRoutingTable *ort, LinkStatePacket **lsp_)
{
  int i;
  int j;
  LinkStatePacket *lsp;

  if ((ort == NULL) || (lsp == NULL)) {
     ERR("Wrong arguments ..\n");
     return -1;
  }

  lsp = (LinkStatePacket *) 
            malloc(sizeof(LinkStatePacket) * ort->numOfRouters);
  if (!lsp) {
     DEBUG("Malloc failed..\n");
     return -1;
  }

  memset(lsp, 0, sizeof(LinkStatePacket)* ort->numOfRouters);
  for (i = 0; i < ort->numOfRouters; i++) {
      lsp[i].lsId = i;
      lsp[i].numOfLinks = 0;
      lsp[i].link = (Link **) malloc(sizeof(Link *) * ort->numOfRouters);
      if (!lsp[i].link) {
            ERR("malloc error %s:%d ..\n", 
                               __FUNCTION__, __LINE__);
      }
      for (j = 0; j < ort->numOfRouters; j++) {
         if (ort->cost[i][j] != -1) { /* ignore non neighbours */
           lsp[i].link[j] = (Link *) malloc(sizeof(Link));
           lsp[i].link[j]->linkid = i*ort->numOfRouters + j;
           lsp[i].link[j]->linkType = 1; /* always same */
           DEBUG("LSP %d %d %d\n", i, j,ort->cost[i][j]); 
           lsp[i].link[j]->linkMetric = ort->cost[i][j];
           lsp[i].numOfLinks++;
         }
      }
  }
  *lsp_ = lsp;
  DEBUG("Link added successfully\n");
  return 0;
}

static int printLSP(LinkStatePacket *lsp, int numOfRouters)
{
  Link *link;
  int i, j; 
  for (i = 0; i < numOfRouters; i++) {
    DEBUG("LSP %d\n", i);
    DEBUG("\tlspId = %d\n", lsp[i].lsId);
    DEBUG("\tnumOfLinks = %d\n", lsp[i].numOfLinks);
    for (j = 0; j < numOfRouters; j++) {
      if (lsp[i].link[j]) {
        DEBUG("\tLink:%d\n", j);
        DEBUG("\t\tlinkid=%d\n", lsp[i].link[j]->linkid);
        DEBUG("\t\tlinkType=%d\n", lsp[i].link[j]->linkType); /* always same */
        DEBUG("\t\tlinkMetric=%d\n", lsp[i].link[j]->linkMetric);
      }
    }
  }
}

void setConfirmedRoute(int src, int dst, int nxt, int cost)
{
  Route *rt;
  if (!rtConfirmed[src].rt[dst]) {
      rtConfirmed[src].rt[dst] = (Route *) malloc(sizeof(Route));
  }
  rt = rtConfirmed[src].rt[dst];
  if (rt) {
      rt->nxtHop = nxt;
      rt->cost = cost;
  }
  return;
}

void setTentativeRoute(int src, int dst, int nxt, int cost)
{
  Route *rt;

  if (!rtTentative[src].rt[dst]) {
      rtTentative[src].rt[dst] = (Route *) malloc(sizeof(Route));
  }
  rt = rtTentative[src].rt[dst];
  if (rt) {
      rt->nxtHop = nxt;
      rt->cost = cost;
  }
  return;
}

Route *getConfirmedRoute(int src, int dst)
{
  return rtConfirmed[src].rt[dst];
}

Route *getTentativeRoute(int src, int dst)
{
  return rtTentative[src].rt[dst];
}

Route *getLeastTentative(int src, int *nxtRtr)
{
  int rid;        
  unsigned int leastCost=0xFFFFFFFF;
  Route *leastCostRt = NULL; 

  DEBUG("Get Least Cost Route ..\n");
  for (rid = 0; rid < ort->numOfRouters; rid++) {
     if ((rtTentative[src].rt[rid]) &&
         ((unsigned )rtTentative[src].rt[rid]->cost < (unsigned)leastCost))
     {
         leastCost = rtTentative[src].rt[rid]->cost;
         *nxtRtr = rid;
         leastCostRt = rtTentative[src].rt[rid];

         DEBUG("leastCost=%d; nxtHop=%d\n",
                         leastCost, *nxtRtr);
     }
  }

  if (leastCostRt) {
    rtTentative[src].rt[*nxtRtr] = NULL;
  }
  return leastCostRt;
}
/* Function to buils routing table using available
 * LinkStatePackets.
 * This function uses Dijkstra's algorithm. 
 */
RoutingTable *buildRoutingTable2(int my_rid)
{
  Route *rtc;
  Route *rtt;
  unsigned int *cost;
  int src_rid;
  int dst_rid;
  int rid;
  cost = (int *) malloc(sizeof(int) * ort->numOfRouters);
  memset(cost, 0, sizeof(int) * ort->numOfRouters);
  cost[my_rid] = 0;
  for (rid = 0; rid < ort->numOfRouters; rid++) {
     if (my_rid != rid) cost[rid] = 0xffffffff;
  }
  cost[my_rid] = 0; 

  setConfirmedRoute(my_rid, my_rid, my_rid, cost[my_rid]);
  LOG ("--> Confirmed Route(%d, %d) is via %d at cost %d\n",
              my_rid+1, my_rid+1, my_rid+1, cost[my_rid] );

  src_rid = my_rid; 

  while (1) {
      LOG("!! Next Source = %d\n", src_rid+1);
      for (rid = 0; rid < ort->numOfRouters; rid++) {
        if (rid == src_rid) {
                LOG("\tSelf Route(%d, %d)\n", src_rid+1, rid+1);
                continue;      
        }

        if (lsp[src_rid].link[rid] != NULL) {      
          rtc = getConfirmedRoute(my_rid, rid);
          rtt = getTentativeRoute(my_rid, rid);
          if ((!rtt) && (!rtc)) {
              /* initialise cost */
              cost[rid] = lsp[src_rid].link[rid]->linkMetric;
              if (src_rid != my_rid) {
                  cost[rid] += cost[src_rid];
              }
              setTentativeRoute(my_rid, rid, src_rid, cost[rid]);
              LOG ("\tNew Tentative Route(%d, %d) is via %d at cost %d\n",
                  my_rid+1, rid+1, src_rid+1, cost[rid] );
          } else if (rtt) {
              /* update cost */
              cost[rid] += lsp[src_rid].link[rid]->linkMetric;
              if (cost[rid] < rtt->cost) {
                /* set this branch as least in tentative list */
                setTentativeRoute(my_rid, rid, src_rid, cost[rid]);
                LOG ("\tUpdate Tentative Route(%d, %d) is via %d at cost %d\n",
                  my_rid+1, rid+1, src_rid+1, cost[rid] );
              } else {
                LOG ("\tCost to %d from %d via %d is %d : Ignoring this route\n",
                   rid+1, my_rid+1, src_rid+1, cost[rid] );
              }

          }       
        }
        else {
            LOG("\tNo route to %d from %d\n", rid+1, src_rid+1);
        }
      }

      rtt = getLeastTentative(my_rid, &rid);
      if (rtt) {  
        setConfirmedRoute(my_rid, rid, rtt->nxtHop, rtt->cost);
        LOG ("--> Confirmed Route(%d, %d) is via %d at cost %d\n",
              my_rid+1, rid+1, rtt->nxtHop+1, rtt->cost );
        src_rid = rid;
        free(rtt);
      } else {
        LOG("No more tentative routes exiting\n");
        break;
      }

  }

  return NULL;
}




RoutingTable *buildRoutingTable(int my_rid)
{
  return NULL;
}

void allocRoutingTables(int numOfRouters)
{
    int i;
    rtConfirmed = (RoutingTable *) malloc(sizeof(RoutingTable)*numOfRouters);
    memset(rtConfirmed, 0, sizeof(RoutingTable)*numOfRouters);
    for (i = 0; i < numOfRouters; i++) {
        rtConfirmed[i].rt = (Route **) malloc(sizeof(Route*)*numOfRouters);
        memset(rtConfirmed[i].rt, 0, sizeof(Route*)*numOfRouters);
    }

    rtTentative = (RoutingTable *) malloc(sizeof(RoutingTable)*numOfRouters);
    memset(rtTentative, 0, sizeof(RoutingTable)*numOfRouters);
    for (i = 0; i < numOfRouters; i++) {
        rtTentative[i].rt = (Route **) malloc(sizeof(Route*)*numOfRouters);
        memset(rtTentative[i].rt, 0, sizeof(Route*)*numOfRouters);
    }
}


void printRoutingTable(int my_rid)
{
  int rid;
  Route *rtc;
  for (rid = 0; rid < ort->numOfRouters; rid++) {
      rtc = getConfirmedRoute(my_rid, rid);
      LOG ("Route(%d, %d) is via %d at cost %d\n",
                  my_rid+1, rid+1, rtc->nxtHop+1, rtc->cost);
  }
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
    LOG("Your choice = %d\n", ch);
    switch (ch) {
      case 1:
      {
        char dataFile[25] = { 0 };
        LOG("Please load original routing table data file\n");
        scanf("%s", dataFile); 
        ort = loadOrigRoutingTable(dataFile);
        if (!ort){ 
          DEBUG("%s:%d error in constructing original routing table\n",
             __FUNCTION__, __LINE__);
          return -1;
        }
        LOG("%s:%d ort=%p &lsp=%p\n",
             __FUNCTION__, __LINE__, ort, &lsp);

        ret = constructLSP(ort, &lsp);
        if (ret != 0) {
          DEBUG("Link State Packets construction failure\n");
        } else {
          LOG("Link State Packets construction success\n");
          printLSP(lsp, ort->numOfRouters);
        }
        
        allocRoutingTables(ort->numOfRouters);
      }
      break;
      case 2:
      {
        int routerId;
        LOG("Please select a router\n");
        scanf("%d", &routerId); 
        routerId--;
        buildRoutingTable2(routerId);
        LOG("The routing table for router %d is\n", routerId);
        printRoutingTable(routerId);  
      }
      break;
      case 3:
      {
        int srcRouterId;
        int dstRouterId;
        LOG("Please input the source and the destination router\n");
        scanf("%d", &srcRouterId); 
        scanf("%d", &dstRouterId); 
        path = computePath(srcRouterId, dstRouterId);
        if (!path){ 
          DEBUG("%s:%d error in constructing path between %d and %d\n",
             __FUNCTION__, __LINE__, srcRouterId, dstRouterId);
          return -1;
        }
        LOG("The path from %d to %d\n", srcRouterId, dstRouterId);
        printRoutePath(path);  
        break;
      }

    }
  }
}
