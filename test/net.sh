#!/bin/bash

# Use netcat to communicate with HAL (on a network interface or a serial device)
#   - Receives data by listening on sepcified IP address and ports
#   - Transmits precanned DATA back to specified adresss and port
#        Note: May transmit on same connection as it receives or can send-back
#        on HAL listening port (but must match the HAL configuration).

# Network conifgurations
IP_ADDR="127.0.0.1"
IP_PORT1=12345
IP_PORT2=12345
IP_PORT3=50000
IP_PORT3h=6788
IP_PORT4=6787
FIF0_NET1="fifo1"
FIF0_NET3="fifo3"
FIF0_NET3h="fifo3b"
FIF0_NET4="fifo4"
IF_NET1="/dev/vcom1"
IF_NET2="/dev/vcom2"
IF_NET3="lo"
IF_NET4="lo"
 
# Packets written as plaintext hexdump (fed into xxd -r -p to create binary)
DATA01="00 01 01 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 11"
DATA02="00 02 02 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 22"
DATA03="00 03 03 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 33"
DATA04="00 04 04 01 00 10 08 6a 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 44"
DATA05="00 00 00 05 00 00 00 05 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 55"
DATA06="00 00 00 06 00 00 00 06 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 66"
DATA07="00 00 00 07 00 00 00 07 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 77"
DATA08="00 00 00 08 00 00 00 08 00 00 00 01 00 00 00 01 01 23 45 67 89 AB CD EF 5E 67 CE DC 00 0D DD 7C 00 00 00 10 00 82 00 A5 00 64 80 00 00 43 C0 00 00 02 00 88"

# Network emuluation: Fifos feed input into netcat; netcat (binary) output onverted to HEX (-w1 avoids line buffering, but ugly)
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
  echo "  2     sent via '$IF_NET3' ($IP_ADDR:udp:$IP_PORT3h) (20 to send-back from port=$IP_PORT3; 3 to send on wrong mux=3)"
  echo "  4     sent via '$IF_NET4' (reuses $IP_ADDR:tcp:$IP_PORT4)"
  echo "  6 & 8 sent via '$IF_NET1' (resues $IP_ADDR:tcp:$IP_PORT1)"
  read APP_INDEX
  case $APP_INDEX in
    2)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT3h prot=UDP: $DATA02"
      echo "$DATA02" | stdbuf -oL xxd -r -p  > $FIF0_NET3h
      ;;
    20)
      echo "Sending back to $IF_NET3 IP=$IP_ADDR send port=$IP_PORT3 prot=UDP: $DATA02"
      echo " **** HAL not listening on that socket, unless configure HAL so it has no: addr_in and port_in ****\n"
      echo "$DATA02" | stdbuf -oL xxd -r -p  > $FIF0_NET3
      ;;
    3)
      echo "Sending to $IF_NET3 IP=$IP_ADDR HAL listening port=$IP_PORT3h prot=UDP: $DATA03"
      echo " **** No HAL map for mux=3 on $IF_NET3, so HAL drops ****\n"
      echo "$DATA03" | stdbuf -oL xxd -r -p  > $FIF0_NET3h
        ;;
    4)
      echo "Sending to $IF_NET4 IP=$IP_ADDR send port=$IP_PORT4 prot=TCP: $DATA04"
      echo "$DATA04" | stdbuf -oL xxd -r -p  > $FIF0_NET4
      ;;
    6)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA06"
      echo "$DATA06" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    8)
      echo "Sending back to $IF_NET1 IP=$IP_ADDR send port=$IP_PORT1 prot=TCP: $DATA08"
      echo "$DATA08" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;

    *)
      echo -n "Unsupported APP-type = $APP_INDEX. "
      ;;
  esac
done
