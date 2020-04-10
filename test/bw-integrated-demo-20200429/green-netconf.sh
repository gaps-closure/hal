sysctl -w net.ipv4.conf.eth0.rp_filter=0
sysctl -w net.ipv4.conf.eth1.rp_filter=0
sysctl -w net.ipv4.conf.all.rp_filter=0
# May need to restart networking here
ip addr del 10.2.1.1/24 dev eth0
ip link set dev eth0 down 
ip addr add 10.2.1.1/24 dev eth0
ip link set eth0 address 00:00:10:02:01:01
ip link set dev eth0 up 

ip addr del 10.3.1.1/24 dev eth1
ip link set dev eth1 down 
ip addr add 10.3.1.1/24 dev eth1
ip link set eth1 address 00:00:10:03:01:01
ip link set dev eth1 up 

ip neigh add 10.2.2.1 lladdr 00:00:10:02:02:01 dev eth0
ip route add 10.2.2.1/32 dev eth0

