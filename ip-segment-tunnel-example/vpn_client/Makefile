CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: vpn_client.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o bin/vpn_client vpn_client.c

clean veryclean:
	$(RM) bin/vpn_client
	sudo ip tuntap del dev mytun mode tun
	sudo ip tuntap del dev mytap mode tap

# take dump of pure bytes received in TUN/TAP interface and transform 
# to format that wireshark can import!
hex hexdump:
	od -Ax -tx1 -v output-client.bin > output-client.hex

removelocalroute:
	sudo ip route del local 10.0.0.1 dev mytun

addlocalroute:	
	sudo ip route add to local 10.0.0.1 dev mytun

run:
	./bin/vpn_client

