/*
 *  ip_route_main.c
 *  Author: Jonatan Schroeder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "ip_route.h"

/* Largest line in the input file. */
#define MAXLINE 1000

int main(int argc, char *argv[]) {
  
  char line[MAXLINE];
  unsigned int netsize;
  unsigned int ip[4];
  int nic;
  unsigned int metric, update_id;
  router_state state;
  
  state = initialize_router();
  
  while(fgets(line, MAXLINE, stdin)) {
    
    if (toupper(line[0]) == 'U') {
      
      if (sscanf(line, "U %u.%u.%u.%u/%u %d %u %u",
                 &ip[0], &ip[1], &ip[2], &ip[3], &netsize, &nic, &metric, &update_id) < 6)
        fprintf(stderr, "Invalid table entry input: %s", line);
      else
        process_update(&state, ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3], netsize,
                       nic, metric, update_id);
    }
    else if (toupper(line[0]) == 'P') {
      
      // Packet inputs are output exactly as they are. This allows piping to part 1.
      printf("%s", line);
    }
    else {
      
      fprintf(stderr, "Invalid input line: %s\n", line);
    }
  }

  destroy_router(state);
  
  return EXIT_SUCCESS;
}
