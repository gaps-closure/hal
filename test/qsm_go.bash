#!/bin/bash
# Start QEMU VMs sharing backend file(s) used as Shared Memory among VMs

# Backend File and QEMU VM definitions
BLK_SIZE_KB=4096              # 4M = 4,194,304 Byes
BLK_COUNT=8
SBF_PREFIX="sbe_file"
COW_SUBSCRIPT="/IMAGES/ubuntu-20.04.2.0-desktop-amd64"
ACCEL="kvm"
if [[ $OSTYPE == 'darwin'* ]]; then
  ACCEL="hvf"
  COW_SUBSCRIPT="ubuntu-20.04.2.0-desktop-amd64"
fi

# QEMU VM node names and vnc/ssh ports (no associative arrays in MAC OS's bash 3.2)
VM_NODE[0]="green"
VM_NODE[1]="orange"
COMMS_PORT[0]=22
COMMS_PORT[1]=23

# Default parameter
DEVICE_COUNT=2
VM_COUNT=2
FORCE=0
KILL=0
LIST=0

usage() {
  echo -e "\nStart one or more QEMU VMs using shared backend file. Usage:"
  echo    "  sudo bash go_sbf.bash [-c count] [-f] [-h] [-k] [-l] [-p filename]"
  echo    "   -c  Number of Nodes VMs created (default = $VM_COUNT)"
  echo    "   -f  Force start by terminating existing qemu processes"
  echo    "   -k  Kill existing qemu processes (but do not start)"
  echo    "   -l  List existing qemu processes (but do not start)"
  echo    "   -p  Path prefix to Shared Backend File (default = $SBF_PREFIX)"
  exit
}

get_options() {
  while getopts 'c:fhklp:' OPTION; do
    case ${OPTION} in
      c)
        VM_COUNT=${OPTARG}
        ;;
      f)
        FORCE=1
        ;;
      h)
        usage
        ;;
      k)
        KILL=1
        ;;
      l)
        LIST=1
        ;;
      p)
        SBF_PREFIX="${OPTARG}"
        ;;
    esac
  done
  shift "$(($OPTIND -1))"      # Use $* variable later to get other paramst
}

ps_check() {
  MY_QEMUS=$(ps aux | grep 'qemu-system-x86' | grep 'vnc :2[2-3]' | grep -v 'grep')
  if [ -n "$MY_QEMUS" ]; then
    PID_LIST=$(echo "$MY_QEMUS" | awk '{print $2}' | tr '\n' ' ')
    echo "QEMU already running... $PID_LIST"
    if  [ $KILL -eq 1 ] || [ $FORCE -eq 1 ]; then
      for P in $PID_LIST
      do
        kill $P
      done
      sleep 1
    else
      exit
    fi
  fi
  if [ $KILL -eq 1 ] || [ $LIST -gt 0 ]; then
    exit
  fi
}

create_backend_file() {
  echo "Zeroizing $((BLK_SIZE_KB * BLK_COUNT)) KiB shared backend file ($1)"
  dd if=/dev/zero  of="$1" bs=${BLK_SIZE_KB}k count=${BLK_COUNT}
# &> /tmp/dd_log_$USER
}

run_qemu() {
  NAME="$1"
  PORT=$2
  SBF="$3"
  SIZE_BYTES=$((BLK_SIZE_KB * BLK_COUNT * 1024))
  
  echo "Starting QEMU VM in enclave $NAME on vnc port $PORT (ssh port 22$PORT), SBF=$SIZE_BYTES Bytes, cow=${COW_SUBSCRIPT}-${NAME}.qcow2"
  qemu-system-x86_64 \
    -hda ${COW_SUBSCRIPT}-${NAME}.qcow2 \
    -smp 2 \
    -vga virtio \
    -usb \
    -device usb-tablet \
    -vnc :$PORT \
    -net nic \
    -net user,hostfwd=tcp::22$PORT-:22 \
    -cpu host \
    -machine type=q35,accel=$ACCEL,nvdimm=on  \
    -m 4G,slots=2,maxmem=10G \
    -object memory-backend-file,id=mem1,share=on,mem-path=$SBF,size=$SIZE_BYTES \
    -device nvdimm,memdev=mem1,id=dimm1,label-size=256K \
    -object memory-backend-file,id=mem2,share=on,mem-path=$SBF,size=$SIZE_BYTES \
    -device nvdimm,memdev=mem2,id=dimm2,label-size=256K &
# mac     -display default,show-cursor=on \
# dram    -machine type=q35,accel=$ACCEL \
# pmem    -machine type=q35,accel=$ACCEL,nvdimm=on  \
# mmem    -machine type=q35,memory-backend=mem1 \
# dram    -device pc-dimm,memdev=mem1,id=dimm1 &
# pmem    -device nvdimm,memdev=mem1,id=dimm1,label-size=256K &
# mmem (no device option)
}

# Main
get_options $@
echo "$VM_COUNT VMs ($COW_SUBSCRIPT) using $DEVICE_COUNT BEFs ($SBF_PREFIX): F=$FORCE, K=$KILL, L=$LIST $*"
ps_check
i=0; while [ $i -lt $DEVICE_COUNT ]; do
  create_backend_file ${SBF_PREFIX}_${COMMS_PORT[$i]}
  i=$((i+1))
done
i=0; while [ $i -lt $VM_COUNT ]; do
  run_qemu ${VM_NODE[$i]} ${COMMS_PORT[$i]} ${SBF_PREFIX}_${COMMS_PORT[$i]}
  i=$((i+1))
done
