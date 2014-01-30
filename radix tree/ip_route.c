/*
 * ip_route.c
 * Author: Anders Linn
 */

#include <stdio.h>
#include <stdlib.h>

#include "ip_route.h"

static inline void print_advertisement(uint32_t ip, uint8_t netsize, int nic,
                                       unsigned int metric, unsigned int update_id) {
  printf("A %u.%u.%u.%u/%u %u %u\n"
         "T %u.%u.%u.%u/%u %d\n",
         (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, netsize, metric, update_id,
         (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, netsize, nic);
}

/* Initializes a node with the given specifications. Doesn't check for duplicates.
 * Fills neighbor table with METRIC_UNREACHABLE entries for all other neighbors.
 */
Node init_node(uint32_t ip, uint8_t netsize, int nic, unsigned int metric) {
	int i;

	Node new = (Node)malloc(sizeof(struct subnet_node));
	new->subnet_ip = ip;
	new->netsize = netsize;
	new->forwarding_nic = nic;
	new->forwarding_metric = metric + 1;
	for(i = 0; i < NUM_NICS; i++) {
		new->neighbor_table[i] = METRIC_UNREACHABLE;
	}
	new->neighbor_table[nic] = metric + 1;
	return new;
}

/* This function initializes the state of the router.
 */
router_state initialize_router(void) {
  
  router_state state = (router_state)malloc(sizeof(struct router_state));
	state->root = (Node)malloc(sizeof(struct subnet_node));
	state->root->next = NULL;
	return state;
}

/* Finds the subnet corresponding to the given ip and netsize. Returns NULL if
 * no such subnet exists.
 */
Node find(router_state *state, uint32_t ip, uint8_t netsize) {
	//implement find
	Node curr = (*state)->root->next;
	int done = 0;
	
	while(curr != NULL && !done) {
	//printf("test3.1\n");
		if(curr->subnet_ip == ip && curr->netsize == netsize) {
			done = 1;
		} else {
			curr = curr->next;
		}
	}
	//printf("test3.2 curr is null?: %i\n",curr == NULL);
	return curr;
}

/* Removes the given node from the list.
 */
void remove_node(router_state *state, Node to_delete) {
	//implement delete
	Node curr, prev;
	prev = (*state)->root;
	curr = prev->next;
	int done = 0;
	
	while(curr != NULL && !done) {
		if(curr == to_delete) {
			prev->next = curr->next;
			free(curr);
			done = 1;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
}

/* Sets forwarding NIC and forwarding metric of a given node based on which neighbor
 * has the smallest distance metric.
 */
int update_min_nic(Node to_check) {
	//implement get_min_nic
	int new_nic = -1;
	unsigned int min_metric = METRIC_UNREACHABLE;
	int i;
	
	for(i = 0; i < NUM_NICS; i++) {
		if(to_check->neighbor_table[i] < min_metric) {
			new_nic = i;
			min_metric = to_check->neighbor_table[i];
		}
	}
	//printf("test4.1\n");
	to_check->forwarding_nic = new_nic;
	to_check->forwarding_metric = min_metric;
	
	return new_nic;
}

// for a faster implementation, just check whether forwarding_metric == METRIC_UNREACHABLE
int is_unreachable(Node to_check) {
	//implement is_unreachable
	int is_unreachable = 1;
	int i;
	
	for(i = 0; i < NUM_NICS; i++) {
		if(to_check->neighbor_table[i] < METRIC_UNREACHABLE) {
			is_unreachable = 0;
		}
	}
	//printf("test5.1\n");
	return is_unreachable;
}

void add_new(router_state *state, uint32_t ip, uint8_t netsize, int nic, unsigned int metric) {
	//implement add_new
	Node curr, prev;
  prev	= (*state)->root;
	curr = prev->next;
	Node new = init_node(ip,netsize,nic,metric);
	int done = 0;
	
	while(curr != NULL && !done) {
	//printf("test2.1\n");
		if(curr->subnet_ip <= ip &&  curr->netsize < netsize) {
			done = 1;
		} else {
			prev = curr;
			curr = curr->next;
		}
	}
	//printf("test2.2\n");
	prev->next = new;
	new->next = curr;
}

void printRouter(router_state *router) {
	Node curr = (*router)->root->next;
	int i = 1;
	int j;
	uint32_t ip;
	
	while(curr != NULL) {
		ip = curr->subnet_ip;
		printf("Entry %i: ip: %u.%u.%u.%u, netsize: %u, fnic: %i, fmetric: %u\n",
			i++,(ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, curr->netsize, 
			curr->forwarding_nic, curr->forwarding_nic);
		for(j = 0; j < NUM_NICS; j++) {
			printf("NIC: %i, metric: %u\n",j,curr->neighbor_table[j]);
		}
		curr = curr->next;
	}
}

/* This function is called for every line corresponding to a routing
 * update. The IP is represented as a 32-bit unsigned integer. The
 * netsize parameter corresponds to the size of the prefix
 * corresponding to the network part of the IP address. The metric
 * corresponds to the value informed by the neighboring router, and
 * does not include the cost to reach that router (which is assumed to
 * be always one). If the update triggers an advertisement, this
 * function prints the advertisement in the standard output.
 */
void process_update(router_state *state, uint32_t ip, uint8_t netsize,
		    int nic, unsigned int metric, unsigned int update_id) {
  
  Node curr = find(state, ip, netsize);
	int old_nic;
	//printf("test1.0 metric: %u metric unreachable: %u\n",metric,METRIC_UNREACHABLE);
	
	if(curr == NULL) {
		if(metric + 1 < METRIC_UNREACHABLE) {
//			printf("test1.1\n");
			add_new(state, ip, netsize, nic, metric);
			//broadcast
			print_advertisement(ip,netsize,nic,metric + 1,update_id);
		}
	} else if(metric + 1 >= METRIC_UNREACHABLE) {
//		printf("test1.2\n");
		curr->neighbor_table[nic] = METRIC_UNREACHABLE;
		if(curr->forwarding_nic == nic) {
//		printf("test1.3\n");
			if(update_min_nic(curr) < 0) {
//			printf("test1.4\n");
				remove_node(state,curr);
				print_advertisement(ip,netsize,-1,METRIC_UNREACHABLE,update_id);
			} else {
				//broadcast
				print_advertisement(ip,netsize,curr->forwarding_nic,curr->forwarding_metric,update_id);
			}
		}
	} else if(nic == curr->forwarding_nic && metric >= curr->forwarding_metric) {
//	printf("test1.5\n");
		curr->neighbor_table[nic] = metric + 1;
		update_min_nic(curr);
//	printf("test1.6\n");
		//broadcast
		print_advertisement(ip,netsize, curr->forwarding_nic, curr->forwarding_metric, update_id);
	} else {
	
//	printf("test1.7\n");
		curr->neighbor_table[nic] = metric + 1;
		
		if(metric + 1 < curr->forwarding_metric) {
//		printf("test1.8\n");
			update_min_nic(curr);
			//broadcast
			print_advertisement(ip, netsize, curr->forwarding_nic, curr->forwarding_metric, update_id);
		} else if( metric + 1 == curr->forwarding_metric && nic < curr->forwarding_nic) {
			curr->forwarding_nic = nic;
			print_advertisement(ip, netsize, curr->forwarding_nic, curr->forwarding_metric, update_id);
		}
	}
	//printRouter(state);
}

/* Destroys all memory dynamically allocated through this state (such
 * as the forwarding table) and frees all resources used by the
 * router.
 */
void destroy_router(router_state state) {
  
  Node curr = state->root;
	Node prev;
	while(curr != NULL) {
		prev = curr;
		curr = curr->next;
		free(prev);
	}
	free(state);
}

