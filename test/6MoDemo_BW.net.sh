ip link set dev grtap0 down
ip link set dev cdgrtap0 down
ip link set dev cdortap0 down
ip link set dev ortap0 down
ip link delete grtap0
ip link delete cdgrtap0
ip link delete cdortap0
ip link delete ortap0
pkill -f "nc -klu 10.0"
pkill -f "nc -u 10.0"

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


#brctl addbr testbr0
#brctl addif testbr0 grtap0
#brctl addif testbr0 ortap0
#iptables -A PREROUTING -t nat -i grtap0 -p udp --dport 50000 -j DNAT --to 10.0.1.2:6788
#iptables -A FORWARD -p udp -d 10.0.1.2 --dport 6788 -j ACCEPT

2>&1 nc -klu 10.0.0.1 50000 | nc -u 10.0.1.2 6788 &
2>&1 nc -klu 10.0.1.1 50000 | nc -u 10.0.0.2 6788 &
