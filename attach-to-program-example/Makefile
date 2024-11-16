CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: tun_alloc.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/tun_alloc tun_alloc.c

clean veryclean:
	$(RM) bin/tun_alloc
	sudo ip tuntap del dev mytun mode tun
	sudo ip tuntap del dev mytap mode tap

# take dump of pure bytes received in TUN/TAP interface and transform 
# to format that wireshark can import!
hex hexdump:
	od -Ax -tx1 -v output-file.bin > output-file.hex

run:
	./bin/tun_alloc

