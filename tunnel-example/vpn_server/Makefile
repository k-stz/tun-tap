CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: tun_alloc.c 
	$(CC) $(CFLAGS) -o tun_alloc tun_alloc.c

clean veryclean:
	$(RM) tun_alloc
	sudo ip tuntap del dev mytun mode tun
	sudo ip tuntap del dev mytap mode tap

# take dump of pure bytes received in TUN/TAP interface and transform 
# to format that wireshark can import!
hex hexdump:
	od -Ax -tx1 -v output-file.bin > output-file.hex

run:
	./tun_alloc

