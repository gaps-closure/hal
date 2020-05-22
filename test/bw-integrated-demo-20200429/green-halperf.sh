cd /home/$SESSION_USER/gaps/build/src/hal/test
LD_LIBRARY_PATH=../appgen/6month-demo ./halperf.py -s 1 1 1 100 -r 2 2 1 -r 2 2 2 -i ipc:///tmp/halsubbwgreen -o ipc:///tmp/halpubbwgreen


