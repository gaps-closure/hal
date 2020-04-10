function list_all {
  if [ -n "$HAL_USER" ]; then echo "HAL being run by $HAL_USER"; fi
  ps aux | grep 'daemon\/hal'
  ps aux | grep 'net\.sh'
  ps aux | grep 'net2\.sh'
  ps aux | grep z[c]
  ps aux | grep [n]etcat
  ip addr show | grep inet | grep tap0
}

function kill_ip {
  sudo ip link set dev grtap0 down
  sudo ip link set dev cdgrtap0 down
  sudo ip link set dev cdortap0 down
  sudo ip link set dev ortap0 down
  sudo ip link delete grtap0
  sudo ip link delete cdgrtap0
  sudo ip link delete cdortap0
  sudo ip link delete ortap0
}
function kill_all {
  ps aux | grep 'daemon\/hal' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep -v $USER | grep 'net\.sh' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux | grep -v $USER | grep 'net2\.sh' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  sudo rm -f /tmp/halsub* /tmp/halpub*
  sudo ip link set dev grtap0 down 2> /dev/null
  sudo ip link set dev cdgrtap0 down 2> /dev/null
  sudo ip link set dev cdortap0 down 2> /dev/null
  sudo ip link set dev ortap0 down 2> /dev/null
  sudo ip link delete grtap0 2> /dev/null
  sudo ip link delete cdgrtap0 2> /dev/null
  sudo ip link delete cdortap0 2> /dev/null
  sudo ip link delete ortap0 2> /dev/null
}

function kill_mine {
  ps aux | grep $USER | grep 'daemon\/hal' | awk '{print $2}' | paste -sd" "  | xargs sudo kill -9 2> /dev/null
  ps aux  | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
  ps aux | grep $USER | grep '[n]et\.sh' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
  ps aux | grep $USER | grep '[n]et2\.sh' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
  ps aux | grep $USER | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
}

# LIST
HAL_USER=$(ps aux | grep 'daemon\/hal' | awk '{print $1}' )
if [ "$1" == "n" ] || [ "$1" == "-n" ]; then
  list_all
  exit
fi

# KILL ALL
if [ "$1" == "f" ] || [ "$1" == "-f" ]; then
  kill_all
  exit
fi

# KILL IP
if [ "$1" == "i" ] || [ "$1" == "-i" ]; then
  kill_ip
  exit
fi

# Warn that someone else is using
if [ -n "$HAL_USER" ] && [ "$HAL_USER" != "$USER" ]; then
  echo "Exiting - Can only have one HAL daemon (and network emulator) per machine: HAL already being run by $HAL_USER"
  list_all
  exit
fi

# Kill my OWN
kill_mine
