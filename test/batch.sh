G_DURATION=10
START=100; STEP=100 END=1400

OFFSET=5
run_g="LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbegreen -o ipc:///tmp/halpubbegreen -r 2 2 1 -t $G_DURATION --interval 10000 -Z -s 1 1 1 "
run_o="LD_LIBRARY_PATH=../appgen ./halperf.py -i ipc:///tmp/halsubbeorange -o ipc:///tmp/halpubbeorange -r 1 1 1 -t $((G_DURATION+OFFSET)) --interval 10000 -z results_halperf.csv -s 2 2 1 "

for i in $(seq $START $STEP $END); do
  eval ${run_o}$i &
  sleep 2
  eval ${run_g}$i
  sleep $OFFSET
  python3 plot_halperf.py -c zz.csv -p yy
  cat zz.csv
done
