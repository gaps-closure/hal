#!/bin/bash

# Create a serial device and connect it to netcat:
#   Display data sent to the device in HEX
#   Send pre-canned inputs to chosen app (based on user input)

IP_ADDR="127.0.0.1"
IP_PORT1=12345
IP_PORT2=12345
IP_PORT3=6789
IP_PORT3b=6788
IP_PORT4=6787
FIF0_NET1="fifo1"
FIF0_NET3="fifo3"
FIF0_NET3b="fifo3b"
FIF0_NET4="fifo4"

IF_NET1="/dev/vcom1"
IF_NET2="/dev/vcom1"
IF_NET3="lo"
IF_NET4="lo"

DATA1="00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 0c 61 62 63 64 65 66 67 68 69 6a 6b 0a"
DATA2="00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 01 00 00 00 0a 6c 6d 6e 6f 70 71 72 73 74 0a"
DATA3="00 00 00 03 00 0a bb bb 6c 6d 6e 6f 70 71 72 73 03 0a"
DATA4="00 00 00 04 00 0a bb bb 6c 6d 6e 6f 70 71 72 73 04 0a"

rm -f $FIF0_NET1 $FIF0_NET3 $FIF0_NET3b $FIF0_NET4

# To get rid o bufering can add (or just send 11 char and CR as the string): -w1 -v
mkfifo $FIF0_NET1
mkfifo $FIF0_NET3
mkfifo $FIF0_NET3b
mkfifo $FIF0_NET4
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT1  < $FIF0_NET1  | od -t x1 &
2>&1 netcat -4 -l -u $IP_ADDR $IP_PORT3  < $FIF0_NET3  | od -t x1 &
2>&1 netcat -4    -u $IP_ADDR $IP_PORT3b < $FIF0_NET3b | od -t x1 &
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT4  < $FIF0_NET4  | od -t x1 &

# When a FIFO is opened for reading, it blocks the calling process (so  socat cannot connect).
# When a FIFO is opened for writing, then the reader is unblocked.
# When the writer is done it closes the FIFO, and the reader gets an EOF; thus also closes.
# To prevent the reader from closing, keep a writer open (must be a better way?):
sleep 90000 > $FIF0_NET1 &
sleep 90000 > $FIF0_NET3 &
sleep 90000 > $FIF0_NET3b &
sleep 90000 > $FIF0_NET4 &

sudo socat -d -d -lf socat_log.txt pty,link=$IF_NET1,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT1,ignoreeof &
sleep 1
sudo chmod 777 $IF_NET1
trap 'kill $(jobs -p)' EXIT

while true; do
  echo "Type APP id to send data to APP: 1 & 2 go via '$IF_NET1'; 3 & 4 go via '$IF_NET3'"
  read APP_INDEX
  case $APP_INDEX in
    1)
      echo "Sending to $IF_NET1 IP=$IP_ADDR port=$IP_PORT1 prot=TCP: $DATA1"
      echo "$DATA1" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    2)
      echo "Sending to $IF_NET2 IP=$IP_ADDR port=$IP_PORT2 prot=TCP: $DATA2"
      echo "$DATA2" | stdbuf -oL xxd -r -p  > $FIF0_NET1
      ;;
    30)
      echo "Sending to $IF_NET3 IP=$IP_ADDR port=$IP_PORT3 prot=UDP: $DATA3"
      echo "$DATA3" | stdbuf -oL xxd -r -p  > $FIF0_NET3
      ;;
    3)
      echo "Sending to $IF_NET3b IP=$IP_ADDR port=$IP_PORT3b prot=UDP: $DATA3"
      echo "$DATA3" | stdbuf -oL xxd -r -p  > $FIF0_NET3b
      ;;
    4)
      echo "Sending to $IF_NET4 IP=$IP_ADDR port=$IP_PORT4 prot=TCP: $DATA4"
      echo "$DATA4" | stdbuf -oL xxd -r -p  > $FIF0_NET4
      ;;
    *)
      echo -n "Unsupported APP-type = $APP_INDEX. "
      ;;
  esac
done
