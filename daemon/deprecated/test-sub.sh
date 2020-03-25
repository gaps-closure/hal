#!/bin/bash

# APP publisher sends to HAL Subscriber
#
# Emulate APP publisher connecting to real HAL:
#   ./test-sub.sh
# Emulated HAL publisher talking to APP subscriber
#   window 1: ./test-pub.sh 1
#   window 2: ./test-pub.sh h

IPC_FILE=ipc://halsub

if [ -z $1 ]; then
  echo "APP zc connects to HAL and publishes 1111... to ZMQ $IPC_FILE"
  echo "11111111111111111111111111" | zc -v -n 1 pub $IPC_FILE
else
  echo "Starting Emulated HAL zc receiving on $IPC_FILE"
  zc -v -b sub $IPC_FILE
fi
