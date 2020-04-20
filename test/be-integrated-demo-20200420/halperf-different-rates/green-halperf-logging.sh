TYPE="bw"
DURATION=100
cd $(pwd | sed 's:test/.*$:test/:')
LD_LIBRARY_PATH=../appgen/6month-demo ./halperf.py -r 2 2 1 -r 2 2 2 -i ipc:///tmp/halsub${TYPE}green -o ipc:///tmp/halpub${TYPE}green -t ${DURATION} -v -s 1 1 1 ${1} > green_${1}.log
