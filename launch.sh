#!/bin/bash
make clean; make
sleep 2

mate-terminal --window --geometry "75x10" -t "Device Setup" -e "./net.sh" \
              --tab --geometry "75x10" -t "tshark" -e "sudo tshark -i lo -f \"port 12345\" -T fields -e data" &

sleep 2

mate-terminal --window --geometry "75x10" -t "HAL" -e "./hal sample.cfg" \
              --tab --geometry "75x10" -t "HAL Config" -e "less sample.cfg" &

mate-terminal --window --geometry "55x10" -t "App 1 (Orange Enclave)" -e "./app.sh 1" &

mate-terminal --window --geometry "55x10" -t "App 2 (Orange Enclave)" -e "./app.sh 2" &
