#!/bin/bash

# Simple emulatation of CDG devices that communicate with HAL
# on a network interface or a serial device.
#
# Netcat process emulated GDG:
#   - Receives data by listening on sepcified IP address and ports
#   - Transmits precanned DATA back to specified adresss and port
#        Normally sends back to a HAL listening port; but also,
#        supports sending back on same connection as it receives.

# export LD_LIBRARY_PATH=~/gaps/top-level/hal/api

# Network Device Names
IF_NET1="/dev/vcom1"      # socat serial device
IF_NET3="lo"              # network udp connection
# Emulated CDG Addresses
IP_ADDR="127.0.0.1"
# Emulated CDG Network Ports
IP_PORT1=12345            # CDG Port for tcp connection with socat serial device
IP_PORT2=6788             # HAL listening port for unidirectional udp connection (optional)
IP_PORT3=50000            # CDG listening port for network udp connection
IP_PORT4=6787             # CDG listening port for network tcp connection
# fifos (to feed into netcat)
FIF0_PORT1="fifo1"        # socat FIFO
FIF0_PORT2="fifo2"        # network udp connection with unidirectional HAL-CDG link
FIF0_PORT3="fifo3"        # network udp connection with  bidirectional HAL-CDG link
FIF0_PORT4="fifo4"        # network tcp connection with  bidirectional HAL-CDG link

# Packets written as plaintext hexdump (fed into xxd -r -p to create binary)
# 1) sdh_bw_v1 (compressed header) packets (xdd3 and xdd4)
DATA_BW1_01="00 01 01 01 00 28 17 e6 5a ba 82 6d c4 a4 52 c0 bb f2 59 9e 07 59 44 40 00 00 00 00 00 80 59 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
DATA_BW1_02="00 02 02 01 00 28 2F E7 5a ba 82 6d c4 a4 52 c0 bb f2 59 9e 07 59 44 40 00 00 00 00 00 80 59 40 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
DATA_BW1_03="00 02 02 02 00 28 c0 83 bc 74 93 18 04 56 f0 bf 79 e9 26 31 08 ac 02 40 00 00 00 00 00 00 e0 3f 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00"
DATA_BW1_04="00 04 04 01 00 10 77 28 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 0e"
DATA_BW1_06="00 06 06 66 00 10 77 28 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 10"
# 2) sdh_be_v1 (with timestamps) packets (xdd1, xdd2, xdd6, xdd7)
DATA_BE1_01="00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 01"
DATA_BE1_02="00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 02"
DATA_BE1_05="00 00 00 05 00 00 00 05 00 00 00 01 00 00 00 65 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 05"
DATA_BE1_06="00 00 00 06 00 00 00 06 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 06"
DATA_BE1_07="00 00 00 07 00 00 00 07 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 07"
DATA_BE1_08="00 00 00 08 00 00 00 08 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 08"

# A) Clean up HAL is ugly - need to clean up
rm -f $FIF0_PORT1 $FIF0_PORT2 $FIF0_PORT3 $FIF0_PORT4
ps aux | grep $USER | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
ps aux | grep $USER | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null

HAL_USER=$(ps aux | grep 'daemon\/hal' | awk '{print $1}' )

if [ -n "$HAL_USER" ] && [ "$HAL_USER" != "$USER" ]; then
  echo "Exiting - Can only have one HAL daemon (and network emulator) per machine: HAL already being run by $HAL_USER"
  ps aux | grep 'daemon\/hal'
  ps aux | grep -v $USER | grep 'net\.sh'
  ps aux | grep z[c]
  ps aux | grep [n]etcat
  exit
  # Should be running HAL as daemon, with each user using 'orthogonal' HAL conig files.
  # Could patch to (support multiple HALs/emulators but recommend not) by changing:
  #  1) IP address:               here and in sample.cfg
  #  2) IF_NET1:                  here and in sample.cfg
  #  3) HAL_IPC_SUB & HAL_IPC_PUB in api/xdcomms.h
  #  4) /dev/gaps_ilip_0_*        in sample.cfg (and insmod)
fi

# If no HAL kill all zcats, netcats,....
if [ -z "$HAL_USER" ]; then
  ps aux | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep -v $USER | grep 'net\.sh' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  sudo rm -f /tmp/halsub* /tmp/halpub*
fi

# B) Emulate network IP devices:
#      Fifos feed input into netcat;
#      od converts netcat (binary) output into HEX
#      Line buffering (can add -w1 to od, which will avoid, but hard to read)
mkfifo $FIF0_PORT1
mkfifo $FIF0_PORT3
mkfifo $FIF0_PORT2
mkfifo $FIF0_PORT4
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT1 < $FIF0_PORT1 | od -t x1 &
2>&1 netcat -4    -u $IP_ADDR $IP_PORT2 < $FIF0_PORT2 | od -t x1 &
2>&1 netcat -4 -l -u $IP_ADDR $IP_PORT3 < $FIF0_PORT3 | od -t x1 &
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT4 < $FIF0_PORT4 | od -t x1 &

# C) Unblock FIFOs
#   When a FIFO is opened for reading, it blocks the calling process (so socat cannot connect).
#   But, if FIFO is opened for writing, then the reader is unblocked.
#   When the writer is done it closes the FIFO, and the reader gets an EOF; thus also closes.
#   To prevent the reader from closing, keep a writer open (must be a better way?):

sleep 90000 > $FIF0_PORT1 &
sleep 90000 > $FIF0_PORT2 &
sleep 90000 > $FIF0_PORT3 &
sleep 90000 > $FIF0_PORT4 &

# D) Create tty serial device (with open permissions), whose output is sent to a TCP address
sudo socat -d -d -lf socat_log.txt pty,link=$IF_NET1,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT1,ignoreeof &
sleep 1
sudo chmod 777 $IF_NET1

# E) Final cleanup: ensures serial device can be written to, and kill all jobs on exiting
trap 'kill $(jobs -p)' EXIT

echo "Listening on $IF_NET1 ($IP_ADDR:$IP_PORT1) and $IF_NET3 ($IP_ADDR:udp:$IP_PORT3, $IP_ADDR:tcp:$IP_PORT4)"
#echo "  1-9   sends via '$IF_NET1' (reuses $IP_ADDR:tcp:$IP_PORT1)"
#echo "  11-19 sends via '$IF_NET3' ($IP_ADDR:udp:$IP_PORT2) [211-219 to reuse $IP_PORT3]"
#echo "  20-   sends via '$IF_NET3' (reuses $IP_ADDR:tcp:$IP_PORT4)"

# F) Ask User if they want to request packet transmission.
#      Input selects HAL tag-mux types (e.g., 2 sends packet with mux=2)
#      Each input/tag-mux is tied to a specific device and addresses (via HAL conifg file)
while true; do
  echo
  echo "Type APP mux_to tag to send data back to HAL (or wait for more input)"
 
  read APP_INDEX
  case $APP_INDEX in
    3111)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT2 prot=UDP: $DATA_BW1_01"
      echo "$DATA_BW1_01" | stdbuf -oL xxd -r -p  > $FIF0_PORT2
      ;;
    3221)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT2 prot=UDP: $DATA_BW1_02"
      echo "$DATA_BW1_02" | stdbuf -oL xxd -r -p  > $FIF0_PORT2
      ;;
    120)
      echo "Sending back to $IF_NET3 IP=$IP_ADDR send port=$IP_PORT3 prot=UDP: $DATA_BW1_02"
      echo " **** HAL not listening on that socket, unless configure HAL for no: addr_in and port_in ****\n"
      echo "$DATA_BW1_02" | stdbuf -oL xxd -r -p  > $FIF0_PORT3
      ;;
    3222)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT2 prot=UDP: $DATA_BW1_03"
      echo "$DATA_BW1_03" | stdbuf -oL xxd -r -p  > $FIF0_PORT2
      ;;
    1553)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_05"
      echo "$DATA_BE1_05" | stdbuf -oL xxd -r -p  > $FIF0_PORT1
      ;;
    4664)
      echo "Sending to $IF_NET3 IP=$IP_ADDR send port=$IP_PORT4 prot=TCP: $DATA_BW1_06"
      echo "$DATA_BW1_06" | stdbuf -oL xxd -r -p  > $FIF0_PORT4
      ;;
    2)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_02"
      echo "$DATA_BE1_02" | stdbuf -oL xxd -r -p  > $FIF0_PORT1
      ;;
    6)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_06"
      echo "$DATA_BE1_06" | stdbuf -oL xxd -r -p  > $FIF0_PORT1
      ;;
    8)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_08"
      echo "$DATA_BE1_08" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    *)
      echo -n "Unsupported APP-type = $APP_INDEX. "
      ;;
  esac
done
