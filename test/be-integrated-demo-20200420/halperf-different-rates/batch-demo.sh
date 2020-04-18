G_DURATION=100
START=100; STEP=100 END=1400

OFFSET=5
cd ~/gaps/build/src/hal/test
run_g="LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbwgreen -o ipc:///tmp/halpubbwgreen -r 2 2 1 -t $G_DURATION --interval 10000 -Z -s 1 1 1 "
run_o="LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbworange -o ipc:///tmp/halpubbworange -r 1 1 1 -t $((G_DURATION+OFFSET)) --interval 10000 -s 2 2 1 "

for i in $(seq $START $STEP $END); do
  eval ${run_o}$i -z results_halperf_$G_DURATION_$i.csv &
  sleep 2
  eval ${run_g}$i
  sleep $OFFSET
  python3 plot_halperf.py -c results_batch.csv -p yy -i results_halperf_$G_DURATION_$i.csv
  cat results_batch.csv
done
