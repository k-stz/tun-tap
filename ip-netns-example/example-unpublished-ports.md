# Example: Connecting to container despite unpublished ports 
Question: Is it possible to connect to a port in a container for which you haven't published the ports first, meaning you didn't run `-p` in your  `docker run -p <port>:<port> ...`? Let's make it work.

Lets run a container that contains a flask webserver listens on port 5000 and binds on ANY_IN (`0.0.0.0`), so on all network interfaces:
```sh
docker run --rm -it app_flask:latest
```

Because we haven't done any port-forwarding with the `-p` option it will in this case listen on:
- `localhost:5000`
- `172.17.0.3:5000`
The `172.17.0.3` IP is on a veth interface. Lets enter the network namespace and inspect it. To interact with a container we'll create a named netnamespace.  For this we determine its PID:
```bash
$ docker inspect -f '{{.State.Pid}}' <container-id>
260159
```
Then create a named netnamespace from the container's namespace.
This is done by creating a symlink in `/var/run/netns/` 
to file handle of the processes' net namespace in `/proc/<pid>/ns/net` (see `man 7 namespaces` for details on those files under `/proc`)

```
$ sudo ln -sf /proc/260159/ns/net /var/run/netns/flaskapp-netns
```
With the named netnamespace we can use the `ip exec netns <named-netns>` command to call commands in the network namespace:

```sh
$ sudo ip netns exec flaskapp-netns bash
# we're inside the network namespace!
```

## Inspecting the network namespace
```sh
$ ip addr
1: lo: <LOOPBACK,UP,LOWER_UP> mtu 65536 qdisc noqueue state UNKNOWN group default qlen 1000
    link/loopback 00:00:00:00:00:00 brd 00:00:00:00:00:00
    inet 127.0.0.1/8 scope host lo
       valid_lft forever preferred_lft forever
    inet6 ::1/128 scope host proto kernel_lo 
       valid_lft forever preferred_lft forever
16: eth0@if17: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue state UP group default 
    link/ether 02:42:ac:11:00:02 brd ff:ff:ff:ff:ff:ff link-netns blue
    inet 172.17.0.2/16 brd 172.17.255.255 scope global eth0
       valid_lft forever preferred_lft forever
```
We get two interfaces:
- localhost, with the ip `127.0.0.1/8` bound to it
- eth0@if17 with the ip `172.17.0.2/16` bound to it
eth0 is veth pair whose counter part is denoted by `@if17` which is an internal kernel reference to it. Because it is situated outside of the flaskapps network namespace, we don't know its name. When you query the network interfaces from the host, thus the global/default network namespace you find its veth pair:

```sh
# Run in the default/global network namespace, meaning _not_ inside the container:
$ ip a
...
17: veth46323f2@if16: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue master docker0 state UP group default 
    link/ether a6:8d:a7:8e:a7:7d brd ff:ff:ff:ff:ff:ff link-netnsid 2
    inet6 fe80::a48d:a7ff:fe8e:a77d/64 scope link proto kernel_ll 
       valid_lft forever preferred_lft forever
...
```

short note on `<veth>@if<number>` syntax:
Here we have the opposite pair veth referred to by the index `17:` which itself points back to the veth in the flaskapp netns: `@if16`. Linux stores all its interfaces in an array, so if you create another veth pair it would have index 18 and 19. Those indices are global, so you can query them with `ip`-cli and see where it is connected to.

Ok, so does that mean that when we send pakets to its ip on the host, that we will receive an answer? Surely not, since what is usually needed is the "port-fowarding"  on container start up with `docker run -p <port>:<port>...`:
```sh
# First lets assign an ip to the host veth
# (not how we don't use the @if16 when referring to it)
$ ip addr add 172.17.0.3/16 dev veth46323f2
```
Now host veth has: `172.17.0.3/16`
contianer veth has: `172.17.0.2/16`

Does it curl?
```
$ curl 172.17.0.2:5000
<!doctype html>
<html>
    <head>
        <title>My Flaskapp</title>
    </head>
```
It works!

Note on routing tables:
Also this works because we've chosen an IP that shares the subnet network ip of the container `172.17.0.0./16`, thus a matching ip route was created. Another IP/Network would have also worked, but we'd have to setup the routing table to advertise `172.17.0.0/16` IP behind the `veth` interface. 
