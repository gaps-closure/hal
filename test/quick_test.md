Basic 2-node testing using Jaga/Liono
=====================================

1. on each node:
   git clone git@github.com:gaps-closure/hal
   cd hal
   git checkout develop
   make

2. start hal:
   jaga$   daemon/hal -l 1 test/sample_zmq_bw_direct_jaga_green.cfg
   liono$  daemon/hal -l 1 test/sample_zmq_bw_direct_liono_orange.cfg


3. run halperf
   jaga (sender):
	cd test
	export LD_LIBRARY_PATH=../appgen/6month-demo/
	./halperf.py -s 1 1 1 1 -i ipc:///tmp/halsubgreen -o ipc:///tmp/halpubgreen


   liono (receiver):
   	cd test
	export LD_LIBRARY_PATH=../appgen/6month-demo/
	./halperf.py -r 1 1 1 -o ipc:///tmp/halpuborange -i ipc:///tmp/halsuborange


***The name of the ZMQ sockets (/tmp/halpub<color>) must be unique -- only one instance can run a a time. To deconflict,
   change the name of the zmq socket in your halconfig, e.g.,

{
   // xdd0: HAL-Application Link
   enabled      = 1;
   id           = "xdd0";
   model        = "sdh_ha_v1";                // HAL Packet format
   comms        = "zmq";                      // Unix IPC socket
   mode_in      = "sub";                      // ZMQ subscriber from APP
   mode_out     = "pub";                      // ZMQ publisher to APP
   addr_in      = "ipc:///tmp/halpubgreen";   // URI for ZMQ pub   -----------> ipc:///tmp/mkhalpubgreen
   addr_out     = "ipc:///tmp/halsubgreen";   // URI for ZMQ sub   -----------> ipc:///tmp/halsubgreen
  },
