#!/bin/bash

# Test serial device using netcat:
#   By default prints read output as a hexdump (default)
#   If add option to command line, then outputs as a string

DEVICE_NAME="/dev/vcom1"
IP_ADDR="127.0.0.1"
IP_PORT=12345
FIFO_NAME="fifo1"

# Generate test string (All ascii chars)
HEX_STRING=""
i=0; while [ $i -lt 256 ]; do
  printf -v HEX "%02x" $i
  HEX_STRING="${HEX_STRING}$HEX "
  i=$((i+1))
done

# Start netcast and print received data (as hex-dump or string)
rm -f $FIFO_NAME
mkfifo $FIFO_NAME
if [ -z $1 ]; then
  2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT < $FIFO_NAME | od -t x1  &
else
  2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT < $FIFO_NAME  &
fi

# Start socat and print received data (as hex-dump or string)
sleep 90000 > $FIFO_NAME &
sudo socat -d -d -lf socat_log.txt pty,link=$DEVICE_NAME,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT,ignoreeof &
sleep 1
sudo chmod 777 $DEVICE_NAME
if [ -z $1 ]; then
  cat ${DEVICE_NAME} | od -t x1 &
else
  cat ${DEVICE_NAME} &
fi

trap 'kill $(jobs -p)' EXIT

# User selects Read of Write test
while true; do
  sleep 1
  echo
  echo "Write or Read from ${DEVICE_NAME}?: w=WRITE r=READ"
  read APP_INDEX
  case $APP_INDEX in
    R|r)
      echo "Write to netcat and read from $DEVICE_NAME"
      echo "${HEX_STRING}" | stdbuf -oL xxd -r -p > fifo1
      ;;
    *)
      echo "Write to $DEVICE_NAME and read from netcat"
      echo "${HEX_STRING}" | stdbuf -oL xxd -r -p > ${DEVICE_NAME}
      ;;
  esac
done
