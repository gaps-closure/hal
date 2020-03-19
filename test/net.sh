#!/bin/bash

# Simple emulatation of CDG devices that communicate with HAL
# on a network interface or a serial device.
#
# Netcat process emulated GDG:
#   - Receives data by listening on sepcified IP address and ports
#   - Transmits precanned DATA back to specified adresss and port
#        Normally sends back to a HAL listening port; but also,
#        supports sending back on same connection as it receives.

# Network Devices
IF_NET1="/dev/vcom1"      # socat serial device
IF_NET3="lo"              # network udp connection
IF_NET4="lo"              # network tcp connection
# Emulated CDG Addresses
IP_ADDR="127.0.0.1"
# Emulated CDG Network Ports
IP_PORT1=12345            # CDG Port for tcp connection with socat serial device
IP_PORT3=50000            # CDG listening port for network udp connection
IP_PORT3h=6788            # HAL listening port for network udp connection (optional)
IP_PORT4=6787             # CDG listening port for network tcp connection
# fifos (to feed into netcat)
FIF0_NET1="fifo1"         # socat FIFO
FIF0_NET3="fifo3"         # network udp connection with bidirectional HAL-CDG link
FIF0_NET3h="fifo3b"       # network udp connection with unidirectional HAL-CDG links
FIF0_NET4="fifo4"         # network tcp connection

# Packets written as plaintext hexdump (fed into xxd -r -p to create binary)
# 1) sdh_bw_v1 (compressed header) packets (xdd3 and xdd4)
DATA_BW1_11="00 0b 0b 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 0b"
DATA_BW1_12="00 0c 0c 01 00 10 5d d6 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 0c"
DATA_BW1_13="00 0d 0d 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 0d"
DATA_BW1_14="00 0e 0e 01 00 10 77 28 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 0e"
DATA_BW1_16="00 10 10 02 00 10 77 28 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 10"
# 2) sdh_be_v1 (with timestamps) packets (xdd1, xdd2, xdd6, xdd7)
DATA_BE1_01="00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 01"
DATA_BE1_02="00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 02"
DATA_BE1_05="00 00 00 05 00 00 00 05 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 05"
DATA_BE1_06="00 00 00 06 00 00 00 06 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 06"
DATA_BE1_07="00 00 00 07 00 00 00 07 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 07"
DATA_BE1_08="00 00 00 08 00 00 00 08 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 08"

# Network emuluation: Fifos feed input into netcat;
# od converts netcat (binary) output into HEX (-w1 avoids line buffering, but ugly)
rm -f $FIF0_NET1 $FIF0_NET3 $FIF0_NET3h $FIF0_NET4
mkfifo $FIF0_NET1
mkfifo $FIF0_NET3
mkfifo $FIF0_NET3h
mkfifo $FIF0_NET4
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT1  < $FIF0_NET1  | od -t x1 &
2>&1 netcat -4 -l -u $IP_ADDR $IP_PORT3  < $FIF0_NET3  | od -t x1 &
2>&1 netcat -4    -u $IP_ADDR $IP_PORT3h < $FIF0_NET3h | od -t x1 &
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT4  < $FIF0_NET4  | od -t x1 &

# When a FIFO is opened for reading, it blocks the calling process (so socat cannot connect).
# When a FIFO is opened for writing, then the reader is unblocked.
# When the writer is done it closes the FIFO, and the reader gets an EOF; thus also closes.
# To prevent the reader from closing, keep a writer open (must be a better way?):
sleep 90000 > $FIF0_NET1 &
sleep 90000 > $FIF0_NET3 &
sleep 90000 > $FIF0_NET3h &
sleep 90000 > $FIF0_NET4 &

# Creat tty serial link with output sent to TCP address
sudo socat -d -d -lf socat_log.txt pty,link=$IF_NET1,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT1,ignoreeof &
sleep 1

# Clean: ensure serial device can be written to, and kill all jobs on exiting
sudo chmod 777 $IF_NET1
trap 'kill $(jobs -p)' EXIT

# User can request packet transmission. Input selects HAL tag-mux types (e.g., 2 sends packet with mux=2)
# Each input/tag-mux is tied to a specific device and addresses
while true; do
  echo "Listening on $IF_NET1 ($IP_ADDR:$IP_PORT1) and $IF_NET3 ($IP_ADDR:udp:$IP_PORT3, $IP_ADDR:tcp:$IP_PORT4)"
  echo "Type APP id to send data to APP"
  echo "  6 & 8 sent via '$IF_NET1' (reuses $IP_ADDR:tcp:$IP_PORT1)"
  echo "  12    sent via '$IF_NET3' ($IP_ADDR:udp:$IP_PORT3h) (120 to reuse $IP_PORT3; 13 to send on wrong mux=13)"
  echo "  14    sent via '$IF_NET4' (reuses $IP_ADDR:tcp:$IP_PORT4)"
  read APP_INDEX
  case $APP_INDEX in
    1)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_01"
      echo "$DATA_BE1_01" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    2)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_02"
      echo "$DATA_BE1_02" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    6)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_06"
      echo "$DATA_BE1_06" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    8)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA_BE1_08"
      echo "$DATA_BE1_08" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    12)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT3h prot=UDP: $DATA_BW1_12"
      echo "$DATA_BW1_12" | stdbuf -oL xxd -r -p  > $FIF0_NET3h
      ;;
    120)
      echo "Sending back to $IF_NET3 IP=$IP_ADDR send port=$IP_PORT3 prot=UDP: $DATA_BW1_12"
      echo " **** HAL not listening on that socket, unless configure HAL for no: addr_in and port_in ****\n"
      echo "$DATA_BW1_12" | stdbuf -oL xxd -r -p  > $FIF0_NET3
      ;;
    14)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT3h prot=UDP: $DATA_BW1_14"
      echo "$DATA_BW1_14" | stdbuf -oL xxd -r -p  > $FIF0_NET3h
      ;;
    16)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT3h prot=UDP: $DATA_BW1_16"
      echo "$DATA_BW1_16" | stdbuf -oL xxd -r -p  > $FIF0_NET3h
      ;;
    30)
      echo "Sending to $IF_NET4 IP=$IP_ADDR send port=$IP_PORT4 prot=TCP: $DATA_BW1_14"
      echo "$DATA_BW1_14" | stdbuf -oL xxd -r -p  > $FIF0_NET4
      ;;
    *)
      echo -n "Unsupported APP-type = $APP_INDEX. "
      ;;
  esac
done
