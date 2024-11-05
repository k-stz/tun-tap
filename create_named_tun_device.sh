#/bin/bash

echo "Attempt to create TUN device (Layer 3)"

sudo ip tuntap add mytun mode tun

#echo "TUN device created: mytun"
#echo "If errors it might have already existed"
echo "run 'ip a' to inspect it"
echo "or to list it: ip tuntap list"
