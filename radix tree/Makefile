CC=gcc
CFLAGS=-Wall -g -Wextra -Wno-unused-parameter -Ofast # -Werror
LDFLAGS=

all: ip_forward ip_route
ip_forward: ip_forward_main.o ip_forward.o
ip_route: ip_route_main.o ip_route.o

ip_forward.o: ip_forward.c ip_forward.h capacity.h
ip_route.o: ip_route.c ip_route.h capacity.h
ip_forward_main.o: ip_forward_main.c ip_forward.h capacity.h
ip_route_main.o: ip_route_main.c ip_route.h capacity.h

clean:
	-rm -rf ip_forward.o ip_route.o ip_forward_main.o ip_route_main.o ip_forward ip_route
