#!/bin/bash
./zc_kill.sh
make clean; make
sleep 2
WSIZE="200x7"
mate-terminal --window --geometry $WSIZE -t "monitor bytes" -e "sudo tshark -nli lo -f \"port 12345 or port 6787 or port 6788 or port 6789\" -T fields -e data" \
                 --tab --geometry $WSIZE -t "monitor fields" -e "sudo tshark -nli lo -f \"port 12345 or port 6787 or port 6788 or port 6789\""

mate-terminal --window --geometry $WSIZE -t "Simple Network Emulation" -e "./net.sh"
                 
sleep 2

mate-terminal --window --geometry $WSIZE -t "HAL Config" -e "less sample.cfg" \
                 --tab --geometry $WSIZE -t "HAL" -e "./daemon/hal -v sample.cfg"

read -n 1 -s -r -p "Press key to start applications"

mate-terminal --window --geometry "45x7" -t "App 1 (Orange Enclave)" -e "./app_test 1"
sleep 10
mate-terminal --window --geometry "45x7" -t "App 2 (Orange Enclave)" -e "./app_test 2" &
sleep 10
mate-terminal --window --geometry "45x7" -t "App 3 (Orange Enclave)" -e "./app_test 3" &
sleep 10
mate-terminal --window --geometry "45x7" -t "App 4 (Orange Enclave)" -e "./app_test 4" &

echo "Applications are waiting for a reply from network emulator"
#                 --tab --geometry "100x10" -t "netstat" -e "watch -n 10 \"netstat -aut4p | grep -e 6789\""


