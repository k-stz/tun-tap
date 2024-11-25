CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: vpn_server.c 
	@ mkdir -p bin
	$(CC) $(CFLAGS) -o bin/vpn_server vpn_server.c

clean veryclean:
	$(RM) vpn_server
	sudo ip tuntap del dev mytun mode tun
	sudo ip tuntap del dev mytun2 mode tun

# take dump of pure bytes received in TUN/TAP interface and transform 
# to format that wireshark can import!
hex hexdump:
	od -Ax -tx1 -v output-server.bin > output-server.hex

run:
	./bin/vpn_server

