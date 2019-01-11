#!/usr/bin/env bash
#set -x

#####################
# customization area
#####################
# How long we will check the log file size? Unit = second
INTERVAL=1
LOG_FILE=report.log
#             M    K    B
SIZE_LIMIT=$((1*1024*1024))
# How many log files will be saved?
ROTATE_NR=2
PIPE='/tmp/minicom.cap'
COLLECT_CMD="cat $PIPE | tee -a $LOG_FILE &"

#####################
# sanity check
#####################
if [ ! -p "$PIPE" ]; then
   echo "$PIPE need to be a pipe file!"
   read -p "Would you like to create $PIPE?[Y/n]" answer
   if [ X"$answer" == X"n" ]; then
      exit
   else
      mkfifo $PIPE
      [ ! -p "$PIPE" ] && { echo "ERROR: make FIFO $PIPE failed"; exit; }
   fi
fi

#####################
# function
#####################
function ctrl_c() {
  echo "Trapped CTRL-C"
  # clean up
  kill -9 $PID
  exit
}

function rotate_file {
  local file=$1
  local i=$2

  [ $i -le 0 ] && return

  while [ $i -gt 1 ]; do
    mv ${file}.$((i-1)) ${file}.$i 2>/dev/null
    i=$((i-1))
  done
  mv ${file} ${file}.$i
}

#####################
# process
#####################
eval $COLLECT_CMD
PID=$(jobs -p)

# trap ctrl-c and call ctrl_c()
trap ctrl_c INT

while true; do
  if [ -f $LOG_FILE ]; then
     log_size=`du -b $LOG_FILE | awk '{ print $1 }'`
     if [ $log_size -gt $SIZE_LIMIT ]; then
        kill -9 $PID
        sync
        rotate_file "$LOG_FILE" $ROTATE_NR 
        echo "---------$(uptime)--------"> $LOG_FILE
        eval $COLLECT_CMD
        PID=$(jobs -p)
     fi
  fi

  sleep $INTERVAL
done
