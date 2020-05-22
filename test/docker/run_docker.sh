docker run \
       --cap-add=NET_ADMIN --device /dev/net/tun:/dev/net/tun \
           --mount "type=bind,src=/home/tchen/hal,dst=/root/hal" \
           -w /root/hal \
	   --name hal \
	   --env USER=root \
           -it test bash

