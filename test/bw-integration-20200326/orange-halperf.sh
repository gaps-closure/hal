cd /home/$SESSION_USER/gaps/hal/test
LD_LIBRARY_PATH=../appgen ./halperf.py -s 2 2 1 1 -s 2 2 2 1 -r 1 1 1 -i ipc:///tmp/halsubbworange -o ipc:///tmp/halpubbworange

