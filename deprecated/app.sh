#!/bin/bash

# Emulate a GAPS application communicating with HAL (using ZMQ)
#     Send data from STDIN (ater adding tag for speciied user) to HALSUB
#     Output data onto STDOUT (after removing tag) received on HALPUB

HALPUB='ipc://halpub'
HALSUB='ipc://halsub'
TAG1='tag-app1-m1-d1'
TAG2='tag-app2-m2-d2'
ZC_CMD="zc/zc -v"
#ZC_COMMON_FLAGS="-v"    # Verbose mode
ZC_COMMON_FLAGS=""

# process script input
case $1 in
  1)
    TAG=$TAG1
    ;;
  2)
    TAG=$TAG2
    ;;
  *)
    echo -n "Unsupported APP-type = $APP_INDEX. "
    echo "Usage: $0 APP-type, where APP-type = {1,2). For example:"
    echo "$0 1"
    exit
    ;;
esac

# Subscribe to HALPUB for this app, writing HAL data (affter removing tag) to stdout
FILTER="${TAG:0:8}"
#echo "TAG=$TAG, Filter=$FILTER"
zc/zc $ZC_COMMON_FLAGS -f $FILTER sub $HALPUB | awk '{print $2}' &
trap 'kill $(jobs -p)' EXIT

# Publish data from stdin (after adding tag for this app) to HALSUB
while true; do
  read DATA
  echo "$TAG $DATA" | zc/zc $ZC_COMMON_FLAGS -dEOF -n 1 pub $HALSUB
done
