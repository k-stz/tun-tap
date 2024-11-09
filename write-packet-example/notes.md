# Creating Packet

We receive IPv4 packets on the TUN interface. They start with a 20 byte header. Here some special offsets (byte place)

- 9th: is the protocol byte
     - "1" = ICMP protocol
- 12-15: Source IP
- 16-19: destination

So lets try to access the desination IP, in order to change and then send the packet on effectively to this IP!

Yep, simply writing the raw buffer to the tun/tap_id it will be send over the network, the kernel does the heaviy lifting!

