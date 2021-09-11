Tom, Graham,

I'm having an issue compiling the NG mission app on the ILIP testbed machine (gaps-2-2). It looks like it could be due to the version of g++.  Would it be possible (and not upset anything you are doing) to put a more recent version of g++ on the gaps-2 testbed machines?

Thanks,
 Tony

Below is the detailed version info on gaps-2 and our jaga machine (we can only compile MA on the latter). 


[closure@gaps-2-2 ~]$ g++ --version
g++ (GCC) 4.8.5 20150623 (Red Hat 4.8.5-39)
Copyright (C) 2015 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

[closure@gaps-2-2 ~]$ hostnamectl
   Static hostname: gaps-2-2
         Icon name: computer-server
           Chassis: server
        Machine ID: 06e0043666df44b8b607e6cd219744ea
           Boot ID: 57e3796b43bb4d99ad1279d4608f9d75
  Operating System: RHEL
       CPE OS Name: cpe:/o:redhat:enterprise_linux:7.6:GA:server
            Kernel: Linux 3.10.0-1127.el7.x86_64.debug
      Architecture: x86-64
[closure@gaps-2-2 ~]$ 


amcauley@jaga:~/gaps/build/apps/eop1/case2/MA_v1.0_src/stage$ g++ --version
g++ (Ubuntu 9.3.0-17ubuntu1~20.04) 9.3.0
Copyright (C) 2019 Free Software Foundation, Inc.
This is free software; see the source for copying conditions.  There is NO
warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

amcauley@jaga:~/gaps/build/apps/eop1/case2/MA_v1.0_src/stage$ hostnamectl
   Static hostname: jaga
         Icon name: computer-server
           Chassis: server
        Machine ID: 9127329a498e4be09a9e32600e3637aa
           Boot ID: 6337603c7b3643468f6213b9043c7eab
  Operating System: Ubuntu 20.04.1 LTS
            Kernel: Linux 5.4.0-59-generic
      Architecture: x86-64





Modify hal_autoconfig.py to support multiple networks

---------------------------------------------------------------
A) Notes
---------------------------------------------------------------
Do not need to change to support multiple APPs
prep.sh script to call hal_autoconfig.py
APPS send all data type (including ack) on one channel - like TCP
 

---------------------------------------------------------------
B) we need to support for 3x5=15 combinations.
---------------------------------------------------------------
3 APPS (Test Cases defined in ~rkrishnan/gaps/build/apps/eop1/case1/annotated)
  amcauley@jaga:~/gaps/build/apps/eop1$ ls case*
   case1 = 
   case2 = 
   case3 =

5 Networks (defined in devices.json) - 
          host(s),      i/f device
   net1 = QEMU/EMU,     socat serial device, linked,   socat
   net2 = jaga,         INET taps            loopback, BW             
   net3 = jaga,         ILIP serial device,  loopback, BE
   net4 = MS testbed,   ILIP serial device,  linked,   BE
   net5 = MIND testbed, INET interfaces,     linked,   BW


---------------------------------------------------------------
C) Now we have 2 combinations
---------------------------------------------------------------

2,1) case2 x net1x (HAL built with large payload for sdh_be_v1)

amcauley@jaga:~/gaps/build/apps/eop1$ cat ~/gaps/build/src/emu/config/deploy/devices.json
{
  "devices": [
    {
      "model":    "sdh_ha_v1",
      "path":     "./zc/zc",
      "comms":    "ipc",
      "mode_in":  "sub",
      "mode_out": "pub"
    },
    {
      "model":    "sdh_be_v1",
      "path":     "/dev/vcom",
      "comms":    "tty"
    }
  ]
}


2,2) case2 x net2

amcauley@jaga:~/gaps/build/apps/eop1$ cat ~rkrishnan/gaps/build/apps/eop1/case2/deploy/localtest/devices_jaga_bw.json
{
  "devices": [
    {
      "model":    "sdh_ha_v1",
      "path":     "./zc/zc",
      "comms":    "ipc",
      "mode_in":  "sub",
      "mode_out": "pub"
    },
    {
      "model":          "sdh_bw_v1",
      "path":           "lo",
      "comms":          "udp",
      "enclave_name_1": "green",
      "listen_addr_1":  "10.111.0.2",
      "listen_port_1":  6788,
      "connect_addr_1": "10.111.0.1",
      "connect_port_1": 6788,
      "enclav_name_2":  "orange",
      "listen_addr_2":  "10.111.0.1",
      "listen_port_2":  6788,
      "connect_addr_2": "10.111.0.2",
      "connect_port_2": 6788
    }
  ]
}


