This was a project in a networking class which simulated the intra-router switching done at the IP layer using longest prefix matching with CIDR addresses. The simulation maintains a forwarding table, forwards packets as well as processing updates about the state of the forwarding table.

There was a competition to see whose implementation could handle the most requests the quickest. This version, using a radix tree to match addresses bit by bit, was able to serve 1 million requests in less than a second. The code for the tree is in ip_forward.c. Most of the framework for this simulation was written by Jonatan Schroeder, the Teaching Assistant for this course; he appears as the author of all files he wrote.

Also, the current version uses inlining and compiler optimization extensively to provide the fastest possible implementation, and no guarantees are made as to the safety or space efficiency of the code (it was for a class competition, after all!).

Code was adapted from https://github.com/openstat/ip-radix-tree