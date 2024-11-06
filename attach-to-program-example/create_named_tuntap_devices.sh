#/bin/bash

echo "Attempt to create TUN and TAP device (Layer 3)"

sudo ip tuntap add mytun mode tun
sudo ip tuntap add mytap mode tap

#echo "TUN device created: mytun"
#echo "If errors it might have already existed"
echo "run 'ip a' to inspect it"
echo "or to list it: ip tuntap list"


echo "Setting administrative state of device to UP"
sudo ip link set mytun up
sudo ip link set mytap up

IP_TUN="10.0.0.1/24"
IP_TAP="10.1.0.50/24"
echo "Setting IP of mytun device to: ${IP_TUN}"
sudo ip addr add "${IP_TUN}" dev mytun
echo "Setting IP of mytap device to: ${IP_TAP}"
sudo ip addr add "${IP_TAP}" dev mytap
