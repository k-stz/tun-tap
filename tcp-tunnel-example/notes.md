# Example goal:
my-packet -> via routing(10.0.0.1/24) -> vpn-client(encapsulate) --tcp-> vpn-server (decapsulate) -> release raw my-packet

# Instructions:
1. create the tun-devices and attach ips to them
`bash create_named_tuntap_devices.sh`
Now you should have mytun and mytun2 devices with IP 10.0.0.1 and 10.0.2.1 respectively
2. Start the vpn_server `cd tun-tap/tcp-tunnel-example/vpn_server` and run it with `make`. It will attach to mytun2 and wait for incoming packets
3. Run the vpn_client `cd tun-tap/tcp-tunnel-example/vpn_client` and run `make` to start it. It will attach to the `mytun` network interface and block waiting for incoming packets. 
4. Now send a packet to the `mytun` interface by pinging an ip from its subnet, not its actual ip address or it will get routed to `loopback` due to kernel default routes (see `ip route show table local`). Anyway send an single ICMP packet to it with: `ping 10.0.0.2 -c1`
5. Look at the output of the vpn_client receving the packet, encapsulating it, esablishing a tcp connection to the vpn_server and sending it there. The vpn_server then receives it, decapsulates it and releases it to the network by writing it to the socket attached to the `mytun2` interface

# Debugging hints

## Dropwatch: Dropreason explanation
You can lookup the meaning behind the drop reasons from the
comments in the code here:
- https://github.com/torvalds/linux/blob/master/include/net/dropreason-core.h#L166

For example the above points to dropreason `IP_NOPROTO`, which apparently means IP is not supported.

## How to show packet stats for an interface?
`ip -stats link show <interface>`

## How to log martian packets?
enable:
`sysctl -w net.ipv4.conf.all.log_martians=1 `
view:
- `sudo journalctl -g martian`
example output:
```
Nov 22 20:55:14 sao kernel: IPv4: martian source 10.0.2.33 from 10.0.0.1, on dev mytun
Nov 22 20:55:15 sao kernel: IPv4: martian source 10.0.2.33 from 10.0.0.1, on dev mytun
Nov 22 20:55:16 sao kernel: IPv4: martian source 10.0.2.33 from 10.0.0.1, on dev mytun
Nov 22 20:55:18 sao kernel: IPv4: martian source 10.0.2.33 from 10.0.0.1, on dev mytun
Nov 22 20:55:19 sao kernel: IPv4: martian source 10.0.2.33 from 10.0.0.1, on dev mytun
```
Anyway after changing the source ip to something publicly routable like `200.1.2.3`, we don't see martian packets anymore but the packet still get dropped with reason:

`IP_NOPROTO`


## drop reason: `NETFILTER_DROP`
This is what is currently shown when we attempt to tunnel packets between TUN-devices...

## What gets dropped?
requests from any internal interface seem to get dropped:
`ping -I <interface> <mytun2-subnet-ip>`
