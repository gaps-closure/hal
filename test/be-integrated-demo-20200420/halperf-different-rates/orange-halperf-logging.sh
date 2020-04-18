cd ~/gaps/build/src/hal/test
LD_LIBRARY_PATH=../appgen ./halperf.py -r 1 1 1 -i ipc:///tmp/halsubbworange -o ipc:///tmp/halpubbworange -t 100 --interval 10000 -Z -s 2 2 1 10 -s 2 2 2 100
