CC      = gcc
CFLAGS  = -g
RM      = rm -f


default: all

all: Hello run

Hello: vpn_client.c 
	$(CC) $(CFLAGS) -o vpn_client vpn_client.c

clean veryclean:
	$(RM) vpn_client
	sudo ip tuntap del dev mytun mode tun
	sudo ip tuntap del dev mytap mode tap

# take dump of pure bytes received in TUN/TAP interface and transform 
# to format that wireshark can import!
hex hexdump:
	od -Ax -tx1 -v output-client.bin > output-client.hex

run:
	./vpn_client

