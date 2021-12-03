# Example 

This example is produced for NGC to demonstrate how to write
directly to HAL in c++, rather than using `libxdcomms`.  

## Assumptions
ZeroMQ 3 must be installed as well as (zmqpp)[https://github.com/zeromq/zmqpp].

On Ubuntu 20.04, this can be installed as follows:

```
sudo apt install libzmq3-dev libzmqpp-dev
```

## Run
sender connects to the HAL pub socket and the receiver connects to the HAL sub socket:

on sender:
(orange)$ ./sender ipc:///tmp/halpuborange

on receiver:
(green)$  ./receiver ipc:///tmp/halsubgreen

## Notes
Be sure to review your HAL configuration and make sure MUX/SEC/TYP and direction of data flow
is compliant with the configuration. This sender/receiver is intended for the following
configuration:

https://github.com/gaps-closure/hal/blob/develop/test/sample_zmq_bw_direct_jaga_green.cfg
https://github.com/gaps-closure/hal/blob/develop/test/sample_zmq_bw_direct_liono_orange.cfg
