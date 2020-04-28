# Batch of loopback tests using BE device (April 28, 2020)

G_DURATION=60       # Duration of each experiment (for green node)
START=100; STEP=100 END=1200

OFFSET=5
CMD="LD_LIBRARY_PATH=../appgen/6month-demo python3 halperf.py"
TYPE="be"
ADDP="ipc:///tmp/hal"
RES="results_halperf_$G_DURATION_"
run_g="$CMD -i ${ADDP}sub${TYPE}green  -o ${ADDP}pub${TYPE}green  -r 2 2 1 -t $G_DURATION            -v -s 1 1 1 "
run_o="$CMD -i ${ADDP}sub${TYPE}orange -o ${ADDP}pub${TYPE}orange -r 1 1 1 -t $((G_DURATION+OFFSET)) -v -s 2 2 1 "
DIR=$(pwd | sed 's:test/.*$:test/:')


# main loop
rm -f summary.csv rm plot_*png orange_*log green_*log
echo "Running halperf test from rate=$START to rate=$END (step=$STEP) packets/sec"
cd "$DIR"
for i in $(seq $START $STEP $END); do
  eval ${run_o}$i > orange_$i.log 2> /dev/null &
  sleep 2
  eval ${run_g}$i > green_$i.log 2> /dev/null
  sleep $OFFSET
  python3 correlate_halperf.py -A green_$i.log -B orange_$i.log -r $i
done
echo
python3 plot_halperf.py -c summary.csv -p
