#!/bin/bash

# APP subscriber for HAL Publisher
#
# Emulate APP subscriber connecting to real HAL:
#   ./test-pub.sh
# Emulated HAL publisher talking to APP subscriber
#   window 1: ./test-pub.sh 1
#   window 2: ./test-pub.sh h

IPC_FILE=ipc://halpub

if [ -z $1 ]; then
  echo "APP zc connects to HAL and subscribes to ZMQ $IPC_FILE"
  zc -v sub $IPC_FILE
elif [ "$1" == "1" ]; then
  echo "APP zc listens and subscribes to ZMQ $IPC_FILE"
  zc -v -b sub $IPC_FILE
else
  echo "Starting Emulated HAL zc"
  echo "Hello world (published by Emulated HAL to APP)" | zc -v -n 1 pub $IPC_FILE
fi
