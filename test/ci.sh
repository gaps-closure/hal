#!/bin/bash

#set -e

echo $PWD

HAL=$PWD
APP=/home/tchen/build/apps/simple

sudo bash ${HAL}/test/kill_hal_uncond.sh


####################################
# The nc processes at the end of this scrript cause
# the CI job not to terminate. Kist put duplicate the
# whole script here and terminate NC when the test is done.
# sudo ${HAL}/test/6MoDemo_BW.net.sh
# 
ip link set dev grtap0 down > /dev/null 2>&1 
ip link set dev cdgrtap0 down > /dev/null 2>&1 
ip link set dev cdortap0 down > /dev/null 2>&1 
ip link set dev ortap0 down > /dev/null 2>&1 
ip link delete grtap0 > /dev/null 2>&1 
ip link delete cdgrtap0 > /dev/null 2>&1
ip link delete cdortap0 > /dev/null 2>&1 
ip link delete ortap0 > /dev/null 2>&1 
pkill -f "nc -klu"
pkill -f "nc -u"

ip tuntap add mode tap grtap0
ip tuntap add mode tap cdgrtap0
ip tuntap add mode tap cdortap0
ip tuntap add mode tap ortap0
ip link set dev grtap0 up
ip link set dev cdgrtap0 up
ip link set dev cdortap0 up
ip link set dev ortap0 up
ip addr add 10.0.0.2/24 dev grtap0
ip addr add 10.0.0.1/24 dev cdgrtap0
ip addr add 10.0.1.1/24 dev cdortap0
ip addr add 10.0.1.2/24 dev ortap0

2>&1 nc -klu 10.0.0.1 50000 | nc -u 10.0.1.2 6788 &
PROCS+=("$!")
disown
2>&1 nc -klu 10.0.1.1 50000 | nc -u 10.0.0.2 6788 &
PROCS+=("$!")
disown
####################################

${HAL}/daemon/hal test/sample_6modemo_bw_green.cfg &
${HAL}/daemon/hal test/sample_6modemo_bw_orange.cfg &

echo starting green
pushd ${HAL}/test
LD_LIBRARY_PATH=../appgen ./halperf.py -v -s 1 1 1 1 -r 2 2 1 -r 2 2 2 -i ipc:///tmp/halsubbwgreen -o ipc:///tmp/halpubbwgreen > /tmp/green-bw.log 2>&1 &
#stdbuf -oL ${APP}/simple -e green -f ${APP}/flows.txt > /tmp/green-bw.log 2>&1 &
PROCS+=("$!")
disown

echo starting orange
LD_LIBRARY_PATH=../appgen ./halperf.py -v -s 2 2 1 1 -s 2 2 2 1 -r 1 1 1 -i ipc:///tmp/halsubbworange -o ipc:///tmp/halpubbworange  > /tmp/orange-bw.log 2>&1 &
#stdbuf -oL ${APP}/simple -e orange -f ${APP}/flows.txt > /tmp/orange-bw.log 2>&1 &
PROCS+=("$!")
disown

popd

sleep 20

sudo bash ${HAL}/test/kill_my_hal.sh

echo ${PROCS[@]}

for i in "${PROCS[@]}"
do
    if ps -p $i > /dev/null
    then
        kill -9 $i 2>&1 > /dev/null
    fi
done
