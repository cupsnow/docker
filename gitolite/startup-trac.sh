#!/bin/bash

log_debug () {
  "echo" -e "\033[36;1m`date '+%y/%m/%d %H%M%S'` DEBUG $*\033[0m"
}

proc_running () {
  for i in "$@"; do
    (ps -ef | grep $i | grep -v -e grep -e $0 > /dev/null) && return
  done
}

# trac
TRAC_CMD=$(cat <<CMDEOL
source /home/trac/bin/activate || exit 255
tracd -p 8000 /home/trac/issuer || exit 255
CMDEOL
)

# lauch function from cli then exit
if [ -n "$1" ]; then
  "$@"
  exit
fi

$TRAC_CMD
