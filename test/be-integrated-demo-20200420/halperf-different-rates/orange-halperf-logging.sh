TYPE="bw"
DURATION=100
cd $(pwd | sed 's:test/.*$:test/:')
LD_LIBRARY_PATH=../appgen/6month-demo ./halperf.py -r 1 1 1 -i ipc:///tmp/halsub${TYPE}orange -o ipc:///tmp/halpub${TYPE}orange -t $((DURATION+5)) -v > orange_${1}.log
