

# About
Source: "Tun/Tap interface tutorial" (2010) by waldner https://backreference.org/2010/03/26/tuntap-interface-tutorial/index.html

Dive into TUN/TAP virtual network devices. Unlike regular network devices, which are backed by a hardware NIC, they are entirely simulated in software, by the linux operating system.

They are, for example, the basis for VPN client/server implementation, allowing packets routed to them to be send to a userspace program, thus enabling tunneling needed for a VPN implementation.
- Source: "VPNs, Proxies and Secure Tunnels Explained (Deepdive)" (2023) by liveoverflow https://www.youtube.com/watch?v=32KKwgF67Ho 

# Instruction
## Create TUN/TAP interface
Create persistent tun/tap virtual network interface named `mytun`. Then afterwards list all network interfaces with `ip addr` and find the newly created `mytun` virtual network interface:
```sh
$ sudo ip tuntap add mytun mode tun`
$ ip addr
...
9: mytun: <POINTOPOINT,MULTICAST,NOARP> mtu 1500 qdisc noop state DOWN group default qlen 500
    link/none 
```

## Configure TUN/TAP interface
Enable the mytun virtual network interface for administrative use
```sh
# doesn't change state...!
sudo ip link set mytun up
```

Configure an IP address:
```
sudo ip addr add 10.0.0.1/24 dev mytun
```
The assignment of a /24 subnet to an interface creates a connected route for the whole range:
```sh
$ ip route
...
10.0.0.0/24 dev mytun proto kernel scope link src 10.0.0.1 linkdown
... 
```
Meaning all request to the IPs in to that network will be routed to our `mytun` interface.

## Test Connection
Monitor all packets to the interface:
```sh
$ sudo tcpdump -i mytun
```
Then ping it:
```sh
$ ping 10.0.0.1
PING 10.0.0.1 (10.0.0.1) 56(84) bytes of data.
64 bytes from 10.0.0.1: icmp_seq=1 ttl=64 time=0.129 ms
64 bytes from 10.0.0.1: icmp_seq=2 ttl=64 time=0.084 ms
64 bytes from 10.0.0.1: icmp_seq=3 ttl=64 time=0.111 ms
^C
--- 10.0.0.1 ping statistics ---
3 packets transmitted, 3 received, 0% packet loss, time 2031ms
rtt min/avg/max/mdev = 0.084/0.108/0.129/0.018 ms
sudo ip addr add 10.0.0.1/24 dev mytun
```

 You see packets are delivered, but the tcpdump above won't show any packets, meaning there is no traffic going through the interface. This is correct, the OS decides that no packets needs to be sent "on the wire" and the kernel itself replies to these pings. That's the same behaviour for another interface's IP like if you have docker running `docker0` or `eth0` etc. (this is not true for localhost though `lo`)
 
# A simple Program: Listen on TUN/TAP IP
Now that we have an TUN/TAP virtual network interface and it even has an IP, can we run a simple server in its subnet?
Sure thing: start a simple server listening on the added IP and a port of your choice and traffic will be routed to it locally!

```
$ go run server.go
```
Regardless if you created a TUN or TAP device: to both you can add an ip address and then bind a port to it by a program.

You will be able to curl your server and receive a response, but surprisingly, the tcpdump won't capture any packets! What gives?

# Program: Attachting to TUN interface


