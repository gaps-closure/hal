#!/bin/bash

IF_NET1="/dev/vcom1"      # socat serial device
IP_ADDR="127.0.0.1"
IP_PORT1=7788
FIF0_PORT1="fifo1"        # socat FIFO
DATA_BE1_01="00 00 00 01 00 00 00 02 00 00"
DATA_BE1_02="00 00 00 03 00"

rm -f $FIF0_PORT1
ps aux | grep $USER | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
ps aux | grep '[s]ocat' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
mkfifo $FIF0_PORT1
2>&1 netcat -4 -l -k $IP_ADDR $IP_PORT1 < $FIF0_PORT1 | od -t x1 -v -w1 -Ad &
sleep 90000 > $FIF0_PORT1 &

sudo socat -d -d -lf socat_log.txt pty,link=$IF_NET1,raw,ignoreeof,unlink-close=0,echo=0 tcp:$IP_ADDR:$IP_PORT1,ignoreeof &
sleep 1
sudo chmod 777 $IF_NET1
cat $IF_NET1 | od -t x1 -w1 -v -Ad &

ps aux | grep $USER | grep [n]etcat
ps aux | grep 'sudo [s]ocat'
ps aux | grep $USER | grep '[o]d -t'

ls $IF_NET1
ls fifo*

C01=$(echo -n "$DATA_BE1_01" | xxd -r -p | wc -c)
C02=$(echo -n "$DATA_BE1_02" | xxd -r -p | wc -c)
echo -n ".......select input "

while true; do
  read APP_INDEX
  case $APP_INDEX in
    0)
      echo "Write $C01 binary bytes to netcat side"
      echo -n "$DATA_BE1_01" | xxd -r -p  > $FIF0_PORT1
      ;;
    1)
      echo "Write $C01 binary bytes  to netcat side (stdbuf -oL)"
      echo -n "$DATA_BE1_01" | stdbuf -oL xxd -r -p  > $FIF0_PORT1
      ;;
    2)
      echo "Write $C02 binary bytes  to netcat side (stdbuf -oL)"
      echo -n "$DATA_BE1_02" | xxd -r -p  > $FIF0_PORT1
      ;;
    5)
      echo "Write $C01 binary bytes  to socat side"
      echo -n "$DATA_BE1_01" | xxd -r -p  > $IF_NET1
      ;;
    6)
      echo "Write $C01 binary bytes  to socat side (stdbuf -oL)"
      echo -n "$DATA_BE1_01" | stdbuf -oL xxd -r -p  > $IF_NET1
      ;;
    7)
      echo "Write $C02 binary bytes  to socat side"
      echo -n "$DATA_BE1_02" | xxd -r -p  > $IF_NET1
      ;;
    *)
      echo "Unsupported Index = $APP_INDEX"
      ;;
  esac
done
