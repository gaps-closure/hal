#!/bin/bash
socat -d -d pty,link=/tmp/vcom1,raw,echo=0 pty,link=/tmp/vcom2,raw,echo=0
# echo "Test" > /tmp/vcom2
# cat /tmp/vcom1
# Test
#
# -lf /tmp/socat.out
# ,ignoreeof,unlink-close=0,echo=0
