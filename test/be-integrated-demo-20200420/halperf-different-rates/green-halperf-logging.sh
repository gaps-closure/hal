cd ~/gaps/build/src/hal/test
LD_LIBRARY_PATH=../appgen ./halperf.py -r 2 2 1 -r 2 2 2 -i ipc:///tmp/halsubbwgreen -o ipc:///tmp/halpubbwgreen -t 105 --interval 10000 -z results_halperf.csv -s 1 1 1 100
