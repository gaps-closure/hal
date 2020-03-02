ps aux | grep $USER | grep z[c] | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
ps aux | grep $USER | grep 'net\.sh' | awk '{print $2}' | paste -sd" "  | xargs kill -9 2> /dev/null
