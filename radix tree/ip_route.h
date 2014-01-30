/*
 *  ip_route.h
 *  Author: Jonatan Schroeder
 */

#ifndef _IP_ROUTE_H_
#define _IP_ROUTE_H_

#include <stdint.h>

#include "capacity.h"

typedef struct subnet_node {
	uint32_t subnet_ip;
	uint8_t netsize;
	int forwarding_nic;
	unsigned int forwarding_metric;
	unsigned int neighbor_table[NUM_NICS];
	struct subnet_node* next;
} *Node;

typedef struct router_state {

	// root is a dummy node which serves as the head of a linked list
  Node root;
  
} *router_state;

router_state initialize_router(void);
void process_update(router_state *state, uint32_t ip, uint8_t netsize,
		    int nic, unsigned int metric, unsigned int update_id);
void destroy_router(router_state state);

#endif
