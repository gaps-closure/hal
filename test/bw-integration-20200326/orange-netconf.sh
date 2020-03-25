sysctl -w net.ipv4.conf.eth0.rp_filter=0
# May need to restart networking here
ip addr del 10.0.2.2/24 dev eth0
ip link set dev eth0 down 
ip addr add 10.0.2.2/24 dev eth0
ip link set eth0 address 00:00:10:00:02:02
ip link set dev eth0 up 

ip addr del 10.0.2.3/24 dev eth1
ip link set dev eth1 down 
ip addr add 10.0.2.3/24 dev eth1
ip link set eth1 address 00:00:10:00:02:03
ip link set dev eth1 up 

ip neigh add 10.0.2.3 lladdr 0:0:10:0:2:3 dev eth1
ip neigh add 10.0.2.2 lladdr 0:0:10:0:2:2 dev eth0
ip neigh add 10.0.1.2 lladdr 0:0:10:0:1:2 dev eth0
ip neigh add 10.0.1.3 lladdr 0:0:10:0:1:3 dev eth1
ip route add 10.0.1.3/32 dev eth1

