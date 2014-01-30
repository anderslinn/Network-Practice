/*
 *  ip_forward_main.c
 *  Author: Jonatan Schroeder
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>

#include "ip_forward.h"

/* Largest line in the input file. */
#define MAXLINE 1000

int main(int argc, char *argv[]) {
  
  FILE *ft_output;
  char *filename;
  char line[MAXLINE];
  unsigned int netsize;
  unsigned int ip[4];
  int nic;
  unsigned int packet_id;
  router_state state;
  
  if (argc == 1) {
    filename = "fwd_table.txt";
  }
  else {
    filename = argv[1];
    if (!strcmp(filename, "-"))
      filename = "/dev/stdout";
  }
  
  ft_output = fopen(filename, "w");
  if (!ft_output) {
    perror("Could not open file for writing");
    return 2;
  }
  
  state = initialize_router();
  
  while(fgets(line, MAXLINE, stdin)) {
    
    if (toupper(line[0]) == 'T') {
      
      if (sscanf(line, "T %u.%u.%u.%u/%u %d",
                 &ip[0], &ip[1], &ip[2], &ip[3], &netsize, &nic) < 6)
        fprintf(stderr, "Invalid table entry input: %s", line);
      else
        populate_forwarding_table(&state, ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3], netsize, nic);
    }
    else if (toupper(line[0]) == 'P') {
      
      if (sscanf(line, "P %u.%u.%u.%u %u",
                 &ip[0], &ip[1], &ip[2], &ip[3], &packet_id) < 5)
        fprintf(stderr, "Invalid packet input: %s", line);
      else
        forward_packet(state, ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3], packet_id);
    }
    else if (toupper(line[0]) == 'A') {
      
      // Advertisements are output exactly as they are. This allows piping from part 2.
      printf("%s", line);
    }
    else {
      
      fprintf(stderr, "Invalid input line: %s\n", line);
    }
  }
  
  print_router_state(state, ft_output);
  destroy_router(state);
  
  fclose(ft_output);
  
  return EXIT_SUCCESS;
}
