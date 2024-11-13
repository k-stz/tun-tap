#/bin/bash

echo "Attempt to create TUN and TAP device (Layer 3)"

sudo ip tuntap add mytun mode tun
sudo ip tuntap add mytun2 mode tun


#echo "TUN device created: mytun"
#echo "If errors it might have already existed"
echo "run 'ip a' to inspect it"
echo "or to list it: ip tuntap list"


echo "Setting administrative state of device to UP"
sudo ip link set mytun up
sudo ip link set mytun2 up

IP_TUN="10.0.0.1/24"
IP_TUN2="10.0.2.1/24"

echo "Setting IP of mytun device to: ${IP_TUN}"
sudo ip addr add "${IP_TUN}" dev mytun
echo "Setting IP of mytun2 device to: ${IP_TUN2}"
sudo ip addr add "${IP_TUN2}" dev mytun2
