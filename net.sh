#!/bin/bash

# Create a serial device and connect it to netcat:
#   Display data sent to the device in HEX
#   Send pre-canned inputs to chosen app (based on user input)

IP_ADDR="127.0.0.1"
IP_PORT=12345
FIFO_NAME="fifo1"
DEVICE_NAME="/dev/vcom1"

DATA1="00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 01 00 00 00 0c 61 62 63 64 65 66 67 68 69 6a 6b 0a"
DATA2="00 00 00 02 00 00 00 02 00 00 00 01 00 00 00 02 00 00 00 0a 6c 6d 6e 6f 70 71 72 73 74 0a"

rm -f $FIFO_NAME

mkfifo $FIFO_NAME
# To get rid o bufering can add (or just send 11 char and CR as the string): -w1 -v
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT < $FIFO_NAME | od -t x1  &

# When a FIFO is opened for reading, it blocks the calling process (so cannot socat cannot connect).
# When a FIFO is opened for writing, then the reader is unblocked.
# When the writer is done it closes the FIFO, and the reader gets an EOF; thus also closes.
# To prevent the reader from closing, keep a writer open (must be a better way?):
sleep 90000 > $FIFO_NAME &

sudo socat -d -d -lf socat_log.txt pty,link=$DEVICE_NAME,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT,ignoreeof &
sleep 1
sudo chmod 777 $DEVICE_NAME
trap 'kill $(jobs -p)' EXIT

while true; do
  echo "Type APP id (1 or 2) to send data back to APP (via $DEVICE_NAME)"
  read APP_INDEX
  case $APP_INDEX in
    1)
      echo "Sending to netcat: $DATA1"
      echo "$DATA1" | stdbuf -oL xxd -r -p  > fifo1
      ;;
    2)
      echo "Sending to netcat: $DATA2"
      echo "$DATA2" | stdbuf -oL xxd -r -p  > fifo1
      ;;
    *)
      echo -n "Unsupported APP-type = $APP_INDEX. "
      ;;
  esac
done




