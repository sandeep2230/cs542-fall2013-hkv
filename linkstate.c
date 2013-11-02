#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int numOfRouters;
  int **cost;
} OrigRoutingTable;

typedef struct {
  int linkid;
  int linkType;
  int linkMetric;
} Link;

typedef struct {
  int lsAge;      /* Unused since we are constructing link state from
                     Original Routing Table */
  int lsId;       /* Unused : for the same reason as above */
  int lsSequence; /* Unused : for the same reason as above */
  int lsLength;   /* Unused : for the same reason as above */
  int numOfLinks;
  Link *links;
} LinkStatePacket;


typedef struct {
  int dest;
  int nxtHop;
  int cost;
} Route;

typedef struct RoutingTable {
  Route *rt;
  struct RoutingTable *next;
  struct RoutingTable *previous;
} RoutingTable;

typedef struct {
  int numOfHops;
  Route *rt;
} RoutePath;

OrigRoutingTable *ort;
LinkStatePacket *lsp; /* Ideally link states are obtained through network advertisements
                         but we construct it from original routing table */ 
RoutingTable *rtTentative;
RoutingTable *rtConfirmed;
RoutingTable *rt;
RoutePath *path;

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

static OrigRoutingTable *loadOrigRoutingTable(char *file)
{
  return NULL;
}

static void constructLSP(OrigRoutingTable * ort, LinkStatePacket **lsp)
{
  *lsp = NULL;
}

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
  int ch=0;
  while (!ch) {
    ch = get_input();
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
        constructLSP(ort, &lsp);
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
