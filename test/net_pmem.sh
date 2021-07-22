#!/bin/bash
socat -d -d pty,link=/tmp/pmem0,raw,echo=0 pty,link=/tmp/pmem1,raw,echo=0
