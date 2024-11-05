CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: tun_alloc.c 
	$(CC) $(CFLAGS) -o tun_alloc tun_alloc.c

clean veryclean:
	$(RM) tun_alloc

run:
	./tun_alloc

