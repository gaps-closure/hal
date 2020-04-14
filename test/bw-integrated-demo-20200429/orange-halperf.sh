cd /home/$SESSION_USER/gaps/build/src/hal/test
LD_LIBRARY_PATH=../appgen/6month-demo ./halperf.py -s 2 2 1 10 -s 2 2 2 100 -r 1 1 1 -i ipc:///tmp/halsubbworange -o ipc:///tmp/halpubbworange

