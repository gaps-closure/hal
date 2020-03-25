#!/bin/bash
ip link set dev grtap0 down > /dev/null 2>&1 
ip link set dev cdgrtap0 down > /dev/null 2>&1 
ip link set dev cdortap0 down > /dev/null 2>&1 
ip link set dev ortap0 down > /dev/null 2>&1 
ip link delete grtap0 > /dev/null 2>&1 
ip link delete cdgrtap0 > /dev/null 2>&1
ip link delete cdortap0 > /dev/null 2>&1 
ip link delete ortap0 > /dev/null 2>&1 
pkill -f "nc -klu"
pkill -f "nc -u"

ip tuntap add mode tap grtap0
ip tuntap add mode tap cdgrtap0
ip tuntap add mode tap cdortap0
ip tuntap add mode tap ortap0
ip link set dev grtap0 up
ip link set dev cdgrtap0 up
ip link set dev cdortap0 up
ip link set dev ortap0 up
ip addr add 10.0.0.2/24 dev grtap0
ip addr add 10.0.0.1/24 dev cdgrtap0
ip addr add 10.0.1.1/24 dev cdortap0
ip addr add 10.0.1.2/24 dev ortap0

2>&1 nc -klu 10.0.0.1 50000 | nc -u 10.0.1.2 6788 &
2>&1 nc -klu 10.0.1.1 50000 | nc -u 10.0.0.2 6788 &
