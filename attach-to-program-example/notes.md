# Trials

## 
tcpdump:
```log
19:55:51.774164 IP sao > 10.0.0.2: ICMP echo request, id 39, seq 1, length 64
```

Important: ICMP is a user of IP, so ICMP packets are encapsulated in IP packets! We will use this to our advantage.

Hexdump 88 bytes from device `mytun`:
```
000000 00 00 08 00 45 00 00 54 22 83 40 00 40 01 04 24
000010 0a 00 00 01 0a 00 00 02 08 00 8a ff 00 29 00 01
000020 4f 0f 2d 67 00 00 00 00 2a 8d 07 00 00 00 00 00
000030 10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f
000040 20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f
000050 30 31 32 33 34 35 36 37
000058
```
Wireshark analysis yields that the IP header starts at `0x45`
- 0x45 = IPv4
- 0x00 = congestion notification
- 0x0054 = length of package 84
- etc. and th 0a 00 00 02 is the destination ip address 10.0.0.2 used to ping the device!

## What are the 4 bytes prior to the IP header?
`00 00 08 00`
These are the flags and herpa derpa set when you create or attach to a TUN/TAP device with the cal `ioctl(fd, TUNSETIFF, (void*) &ifr)` where for `ifr` you set the flags `ifr.ifr_flags = IFF_TUN`

To fix this set the addition flag `IFF_NO_PI` like so:
`ifr.ifr_flags = IFF_TUN | IFF_NO_PI`

Citing the documentation for the Linux tun/tap kernel drive:
https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/Documentation/networking/tuntap.rst?id=HEAD
```
3.2 Frame format
----------------

If flag IFF_NO_PI is not set each frame format is::

     Flags [2 bytes]
     Proto [2 bytes]
     Raw protocol(IP, IPv6, etc) frame.
```
So the 00 00 08 00 are flags and proto. 