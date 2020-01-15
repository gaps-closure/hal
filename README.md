This directory will include the Hardware Abstraction Layer (HAL) Service which
abstracts the TA1 API for use by the cross-domain programs. The HAL Service
runs as a daemon (typically started by a systemd script at boot time).  

It allows application to send and receive cross-domain data and tags over
ZeroMQ. It opens and manages cross domain guard devices (real or emualted). It
mediates the exchange between the application and the guard devices, handling
the encoding/decoding and multiplexing/demultiplexing as applicable.

Make sure to install pre-requisites.
```
sudo apt install -y libzmq3-dev
sudo apt install -y libconfig-dev
```

# TESTING WITH SCRIPTS

Run 5 scripts (each in separate windows) to start the monitor, netwrok device, HAL, and 2 APPs  
```
# 1) Start the monoitor
sudo tshark -i lo -f "port 12345" -x 

# 2) Start the network device 
./net.sh

# 3) Start HAL (run 'make' if hal.c or zc.c has been modified)
./hal sample.cfg

# 4) Start APPs 1 and 2
./app 1
./app 2
```

# RAW TESTING

Testing with a socat device sending and receiving BKEND Format binary data

A) Setup (run each command in a separate window).
```
# 1) netcat listens on localhost port 1234 and converts packets to and from hex format
#    on stdout and stdin
stdbuf -oL xxd -r -p | netcat -4 -l -k 127.0.0.1 1234 2>&1 | od -t x1 -w1 -v

# 2) Start a serial device (/dev/vcom1) that communicates to netcat
sudo socat -d -d -lf socat_log.txt pty,link=/dev/vcom1,raw,ignoreeof,unlink-close=0,echo=0 tcp:127.0.0.1:1234,ignoreeof
sudo chmod 777 /dev/vcom1

# 3) Start HAL and 2 Applications (app1 and app2) emulationed with zc
#    Note: Make sure the socat device (/dev/vcom1) is enabled in sample.cfg.
./hal sample.cfg
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v -f 'tag-app1' sub ipc://halpub
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v -f 'tag-app2' sub ipc://halpub
#    Note: Publisher or app1 and app2 are the same (just use different tags - see below)
#          Add more zc pub-sub pairs (with new sub filter) to emulate further apps
```

B) Passing data ( between app and device)
```
# 1) Send from APP by typing in a zc halsub window, using a haljson formated tag and string.
#    (three '-' separated tag strings [mux, sec, typ], a space. and a data string)
#    For example, to emulate sending from app1:
tag-app1-m1-d1 ABCDEFGHIJK
#    To emulate sending from app1:
tag-app2-m2-d2 LMNOPQRST
#    Note: Output from both apps will be printed in the netcat window (in hex)

# 2) Send from device by typing in the netcat window, using a bkend formated hex data
#    (Five uint32_t (4-byte) tags [mux, sec, count, typ, len] followed by the data)
#    For example, to emulate sending to app1:
00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 0c 61 62 63 64 65 66 67 68 69 6a 6b 0a
#    To emulate sending to app2:
00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 02 00 00 00 0a 6c 6d 6e 6f 70 71 72 73 74 0a
#    Note: Output will be printed in the APP zc halpub window (as a string)
```


Loopback APP-side testing (without a device)
```
# 1) Start the HAL and the Application(s) send and receive emulation (using zc)
./hal sample_zc_loopback.cfg
zc/zc -v -dEOF pub ipc://halsub
zc/zc -v sub ipc://halpub
#    Note: sample_zc_loopback.cfg disables all devices and routes from zc to zc
```

**This directory needs to be moved out of emulator into its own repository, as it
is a CLOSURE component that applies to both real and emulated systems.**
