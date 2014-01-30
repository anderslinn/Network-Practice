/*
 *  ip_forward.h
 *  Author: Jonatan Schroeder
 */

#ifndef _IP_FORWARD_H_
#define _IP_FORWARD_H_

#include <stdint.h>

#include "capacity.h"

// holds ip and netsize information for a radix trie node
typedef struct info_struct {
	uint32_t ip;
	uint8_t netsize;
} *info;

// node of a radix trie for finding ip addresses
typedef struct trie_node {
	
	int nic;
	info info;
	struct trie_node *left;
	struct trie_node *right;
	
} *Node;

// pointer to the root of a prefix trie
typedef struct router_state {
  
  Node root;
  
} *router_state;

router_state initialize_router(void);
void populate_forwarding_table(router_state *state, uint32_t ip, uint8_t netsize, int nic);
void forward_packet(router_state state, uint32_t ip, unsigned int packet_id);
void print_router_state(router_state state, FILE *output);
void destroy_router(router_state state);

#endif
