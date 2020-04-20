G_DURATION=100
START=100; STEP=100 END=1400

OFFSET=5
CMD="LD_LIBRARY_PATH=../appgen/6month-demo ./halperf.py"
TYPE="be"
ADDP="ipc:///tmp/hal"
RES="results_halperf_$G_DURATION_"
INT="--interval 10000"
run_g="$CMD -i ${ADDP}sub${TYPE}green  -o ${ADDP}pub${TYPE}green  -r 2 2 1 -t $G_DURATION $INT -Z -s 1 1 1 "
run_o="$CMD -i ${ADDP}sub${TYPE}orange -o ${ADDP}pub${TYPE}orange -r 1 1 1 -t $((G_DURATION+OFFSET)) $INT -s 2 2 1 "
DIR=$(pwd | sed 's:test/.*$:test/:')

# main loop
cd "$DIR"
for i in $(seq $START $STEP $END); do
  eval ${run_o}$i -z ${RES}$i.csv &
  sleep 2
  eval ${run_g}$i
  sleep $OFFSET
  python3 plot_halperf.py -c ${RES}batch.csv -p results -i ${RES}$i.csv
  cat ${RES}batch.csv
done
