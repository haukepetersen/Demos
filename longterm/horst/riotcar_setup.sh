#!/bin/sh

sudo ip a a 2001:affe:0815::1 dev eth0
sudo ip r a 1234::1 dev eth0
sudo ip r a beef::/64 via 1234::1 dev eth0
