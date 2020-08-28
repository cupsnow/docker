#!/bin/bash

log_debug () {
  "echo" -e "\033[36;1m`date '+%y/%m/%d %H%M%S'` DEBUG $*\033[0m"
}

proc_running () {
  for i in "$@"; do
    (ps -ef | grep $i | grep -v -e grep -e $0 > /dev/null) && return
  done
}

# ssh
SSH_EXEC="/usr/sbin/sshd"
ssh_svc () {
  $SSH_EXEC
}

# lauch function from cli then exit
if [ -n "$1" ]; then
  "$@"
  exit
fi

# start service
ssh_svc

# keep this foreground
while sleep 5; do
  proc_running "$SSH_EXEC" && continue
done
