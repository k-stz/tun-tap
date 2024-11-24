# Example goal:
my-packet -> via routing(10.0.0.1/24) -> vpn-client(encapsulate) --tcp-> vpn-server (decapsulate) -> release raw my-packet



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
