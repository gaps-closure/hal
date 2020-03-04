ps aux | grep $USER | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
ps aux | grep $USER | grep '[n]et\.sh' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
ps aux | grep $USER | grep '[n]etcat' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
