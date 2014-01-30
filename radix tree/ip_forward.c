/*
 * ip_forward.c
 * Author: Anders Linn
 */

#include <stdio.h>
#include <stdlib.h>

#include "ip_forward.h"

/** Helper function that prints the output of a frame being forwarded. */
static inline void print_forwarding(unsigned int packet_id, int nic) {
  printf("O %u %d\n", packet_id, nic);
}

/** Helper function that prints a forwarding table entry. */
static inline void print_forwarding_table_entry(uint32_t ip, uint8_t netsize, int nic, FILE *output) {
  fprintf(output, "%u.%u.%u.%u/%u %d\n",
          (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF, netsize, nic);
}

/** This function initializes a node of a prefix trie.
 */
static inline Node init_node(int nic, info info) {
	Node new = (Node)malloc(sizeof(struct trie_node));
	new->nic = nic;
	new->info = info;
	return new;
}

/** This function initializes an info struct to hold the ip and netsize information
 *  for a node of a prefix trie.
 */
static inline info init_info(uint32_t ip, uint8_t netsize) {
	info new = (info)malloc(sizeof(struct info_struct));
	new->ip = ip;
	new->netsize = netsize;
	return new;
}

/** This function initializes the state of the router.
 */
router_state initialize_router(void) {
  
  router_state state = (router_state)malloc(sizeof(struct router_state));
	state->root = init_node(-1,NULL);
	return state;
	
}

/** Finds the best nic to forward a packet bound for a given ip address. If no rule
 *  exists, returns -1 to indicate "broadcast"
 */
static inline int find(router_state state, uint32_t ip) {

	/* 32-bit number with a single 1 at the beginning. This is used to check
		 each bit of ip (a CIDR block address) one at a time. */
	uint32_t bit = 0x80000000;
	/* Starting value for nic. If no matching entry is ever found, nic will never be
	   changed and the function will return -1 for "broadcast". */
	int nic = -1;
	/* Pointer to traverse the trie. */
	Node curr = state->root;
	
	/* Follows the trie down as far as it can, recording the best match along the way.
	   Since the children of a node always have longer prefixes than their parent (due
		 to the way the tree is structured), whatever the nic of the currently matching
		 node is represents the longest prefix matched so far. The pointer is updated by
		 bitwise ANDing the ip address with the active bit of the bit variable; if the 
		 result is a zero, the left child is chosen as the next node, otherwise the right
		 child is chosen. */
	while(curr != NULL) {
		// save the nic if it matches
		if(curr->nic != -1) {
			nic = curr->nic;
		}
		// determine which child to check next
		curr = ((ip & bit) != 0) ? curr->right : curr->left;
		// move the active bit
		bit >>= 1;
	}
	
	// return the nic of the longest matching prefix, or -1 if no prefix was matched
	return nic;
}

/** Adds and removes entries from a prefix trie. At each node of the trie, information
 *  regarding the ip and subnet at that node will be stored if it has been added previously.
 *  The children of a node correspond to the result of considering one more bit than
 *  would be included in the netsize highest order bits of that node (ie, node->info->netsize).
 *  The left child handles the case in which that bit is a zero; the right child handles
 *  the case where the bit is a one. In this way, every 32-bit string of ones and zeros
 *  has a unique location in the trie, though only a subset of them need ever be allocated.
 */
static inline void alter(router_state *state, uint32_t ip, uint8_t netsize, int nic) {
		
	/* 32-bit number with a single 1 at the beginning. This is used to check
		 each bit of ip (a CIDR block address) one at a time. */
	uint32_t bit = 0x80000000;	
	/* 32-bit mask with netsize 1's. Used with bit to limit the search to the
		 first netsize bits of the ip. */
	uint32_t mask = netsize == 0 ? 0 : 0xffffffff << (32 - netsize);
	/* Pointers to traverse the trie. next is used to traverse the children of
		 nodes in the trie. curr is used to keep track of the parent of next, so
		 that when NULL children are encountered there is no need to retraverse 
		 the tree to find them. */
	Node curr = (*state)->root;
	Node next = curr;
	
	/* The active bit (the 1) in variable bit is one of the netsize highest 
		 order bits. Each time around the loop, the value of the bit in
		 the given ip address which occupies the same position (is the same 
		 order) as the active bit is checked. If it's a zero, then next is 
		 updated to point to the left child of curr (the current node in the 
		 trie). Otherwise, next is updated to point to the right child of curr.
		 
		 If the desired child doesn't exist, no further nodes exist along the path 
		 to ip, and therefore no entry for ip currently exists. The loop exits 
		 prematurely with curr pointing to the closest matching location to ip.

		 If the child does exist, the active bit is shifted one place further down
		 (to the right on a big-endian machine), the child becomes the current node,
		 and the loop continues.
		 
		 The exit condition for the loop (if not exited prematurely) is that the 
		 active bit is no longer one of the netsize highest order bits. Since the
		 loop has not been exited, and the bits of ip were matched in order at each
		 step, then the current node must be the entry for the given ip and netsize.
		 The loop terminates with curr pointing to this entry. */
	while((bit & mask) != 0) {
		// check bit
		next = ((ip & bit) != 0) ? curr->right : curr->left;
		// check if next child exists
		if(next == NULL) {
			break;
		}
		// move the active bit one place and update the current node
		bit >>= 1;
		curr = next;
	}
	
	/* If the loop terminated naturally, then curr and next will be pointing to the
		 node corresponding to ip/netsize. The nic for this node is updated; if the
		 nic is -1, then the node is being removed. If it has an info struct allocated,
		 the struct is freed. If the nic is not -1, then the node is being updated. If it
		 has no info struct allocated currently, then it is being added to the tree, either
		 for the first time or after previous removal. An info struct is allocated for it,
		 holding the ip and netsize information for the node.
		 
		 If the loop terminated prematurely, then next will be pointing to NULL and the ip
		 needs to be added. Dummy nodes will be inserted along the path to ip, each with a 
		 nic of -1 to indicate that they are not real entries.

		 Note that if nic == -1 and the node doesn't exist, there is no need to do anything!
		 */
	if(next != NULL) {
		// ip found, update nic and info
		curr->nic = nic;
		if(nic == -1 && curr->info != NULL) {
			// make entry inactive
			free(curr->info);
			curr->info = NULL;
		} else if(nic != -1 && curr->info == NULL) {
			// (re)activate entry
			curr->info = init_info(ip,netsize);
		}
	} else if(nic != -1) {
	
		/* Dummy entries are added for each bit left in the netsize highest order bits of
			 the ip address. This ensures that future searches will find the existing entry 
			 and maintains the structure of the tree at the cost of some memory overhead. 
			 However, these dummy entries correspond to possible CIDR block addresses, so in
			 the future they can be found and activated if necessary. This offsets the cost
			 of creating them to some extent. 
			 
			 The loop works as above, traversing through the rest of the netsize highest order
			 bits of ip. At each step, a dummy node is created and assigned to one of the
			 children of the current node, which becomes the current node for the next iteration. */
		while((bit & mask) != 0) {
			// initialize a dummy node
			next = init_node(-1,NULL);
			
			// check whether to go left or right by bitwise ANDing ip with the active bit
			if((ip & bit) != 0) {
				curr->right = next;
			} else {
				curr->left = next;
			}
			
			// move the active bit
			bit >>= 1;
			// set the newly created child to be the current node
			curr = next;
		}
		
		// node found, allocate info and update nic
		curr->info = init_info(ip,netsize);	
		curr->nic = nic;
	}
}	

/** This function is called for every line corresponding to a table
 *  entry. The IP is represented as a 32-bit unsigned integer. The
 *  netsize parameter corresponds to the size of the prefix
 *  corresponding to the network part of the IP address. Nothing is
 *  printed as a result of this function.
 */
inline void populate_forwarding_table(router_state *state, uint32_t ip, uint8_t netsize, int nic) {
  
	alter(state,ip,netsize,nic);
	
}

/** This function is called for every line corresponding to a packet to
 *  be forwarded. The IP is represented as a 32-bit unsigned
 *  integer. The forwarding table is consulted and the packet is
 *  directed to the smallest subnet (longest prefix) in the table that
 *  contains the informed destination IP. A line is printed with the
 *  information about this forwarding (the function print_forwarding is
 *  called in this case).
 */
inline void forward_packet(router_state state, uint32_t ip, unsigned int packet_id) {
  
	int nic = find(state,ip);
  print_forwarding(packet_id, nic);
	
}

/** Prints a subtrie of a prefix trie in order (lowest ip first, lowest netsize first
 *  when ip's are tied).
 */
static inline void print_subtrie(Node node, FILE *output) {

	/* Since ip addresses are assessed one bit at a time starting from the highest order
		 bit and terminating after netsize bits have been checked, entries with shorter prefixes
		 will tend to be higher up in the trie. Since the decision at each node is to go left for
		 zero and right for one, nodes with higher ip addresses will tend to be towards the right of
		 the trie. To print the router state, do a pre-order traversal of the trie. */
	if(node != NULL) {
		if(node->info != NULL) {
			// only print entries that exist (have an actual nic)
			print_forwarding_table_entry(node->info->ip, node->info->netsize, node->nic, output);
		}
		print_subtrie(node->left, output);
		print_subtrie(node->right, output);
	}
}

/** Prints the current state of the router forwarding table. This
 *  function will call the function print_forwarding_table_entry for
 *  each valid entry in the forwarding table, in order of prefix
 *  address (or in order of netsize if prefix is the same).
 */
void print_router_state(router_state state, FILE *output) {
	
	print_subtrie(state->root, output);
  
}

/** Deletes a subtrie of a prefix trie. Frees all allocated info structs
 *  along the way.
 */
static inline void delete_subtrie(Node node) {
	if(node != NULL) {
		delete_subtrie(node->left);
		delete_subtrie(node->right);
		if(node->info != NULL) {
			free(node->info);
		}
		free(node);
	}
}

/** Destroys all memory dynamically allocated through this state (such
 *  as the forwarding table) and frees all resources used by the
 *  router.
 */
void destroy_router(router_state state) {
  
	delete_subtrie(state->root);
	free(state);
}



