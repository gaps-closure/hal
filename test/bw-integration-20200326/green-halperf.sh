cd /home/$SESSION_USER/gaps/hal/test
LD_LIBRARY_PATH=../appgen ./halperf.py -s 1 1 1 1 -r 2 2 1 -r 2 2 2 -i ipc:///tmp/halsubbwgreen -o ipc:///tmp/halpubbwgreen

