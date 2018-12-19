#!/usr/bin/env bash
#set -x
###################
# Definition
###################
: ${SCRIPT_NAME:=$0}
NCDU_RES=''
NCDU_DIR=''
RES_DIR='/tmp'
ALL_RES="$RES_DIR/ncdu.res"
TMUX_SESSION_NAME='ncdu_tmux_session'
TOP_MAX=3
APP_LIST='tmux ncdu gzip zcat'
MAIL_TO=''
USE_GZ=0
WAIT_INTERVAL=1
HEADER=""
FOOTER=""

##################
# Function
##################
function is_directory {
  local row=$1
  # remove prefix like '  108.8MiB [ 91.8% ##########]'
  row="${row#*]}"
  # to trim leading and trailing whitespace
  row="$(echo -e "${row}" | sed -e 's/^[[:space:]]*//' -e 's/[[:space:]]*$//')"
  # in the output of ncdu, the directory entry start from '/'
  if [ "${row:0:1}" = '/' ]; then
     return 1
  else
     return 0
  fi
}

function check_apps {
  for a in $1 ; do
      which $a 2>&1 >/dev/null
      if [ $? -ne 0 ]; then
         echo "$SCRIPT_NAME: Please install \"$a\"."
         exit 1
      fi
  done
}

function tmux_capture {
  #tmux capture-pane -t ${TMUX_SESSION_NAME} \; save-buffer -b 0 "$1" \; delete-buffer -b 0 \;
  tmux capture-pane -t ${TMUX_SESSION_NAME} \; save-buffer "$1" \; delete-buffer \;
}

function wait_draw {
  local tmp_res=$1
  local out=0
  until [ $out -eq 1 ]; do
    tmux_capture "${tmp_res}"
    sed -i -e '/^\S*$/d' ${tmp_res}
    grep "Total disk usage:" ${tmp_res} 2>&1 >/dev/null
    [ $? -eq 0 ] && out=1 || sleep $WAIT_INTERVAL
  done
}

function show_help () {
  echo "Usage: $1 [OPTION]..."
  echo -e "\nSupport options:"
  echo "    -f FILE         the ncdu output FILE would be generated by ncdu -o command,"
  echo "                    e.g, ncdu -0xo /tmp/ncdu.disk_usage /path/to/directory/disk_usage"
  echo "    -d DIRECTORY    the DIRECTORY to calculate disk usage."
  echo "    -m EMAIL        send report to EMAIL."
  echo "                    without this option the result will be printed on STDOUT"
  echo "    -t NUMBER       report the top NUMBER entry."
  echo "    -g              the ncdu output file is compressed by gzip"
  echo "    -h              show help info."
}

#################
# Process
#################

check_apps "$APP_LIST"

# Check argument
while getopts :f:d:m:t:gh arg
do
  case $arg in
           #indicate ncdu output
       f)  if [ ! -f "$OPTARG" ]; then
              echo -e "\033[7m$OPTARG\033[0m should be a file." 1>&2
              echo "Usage: $SCRIPT_NAME -f <ncdu output>" 1>&2
              exit 1
           else
              NCDU_RES="$OPTARG"
           fi
           ;;

       d)  if [ -d "$OPTARG" ]; then
              NCDU_DIR="$OPTARG"
           else
              echo -e "\033[7m$OPTARG\033[0m is not a directory." 1>&2
              echo "Usage: $SCRIPT_NAME -d <directory>" 1>&2
              exit 1
           fi
           ;;

       m)  if [ -n "$OPTARG" ]; then
              MAIL_TO="$OPTARG"
              check_apps "mail"
           else
              echo "Usage: $SCRIPT_NAME -m email@example.com" 1>&2
              exit 1
           fi
           ;;

       t)  if [ -n "$OPTARG" ]; then
              TOP_MAX="$OPTARG"
           else
              echo "Usage: $SCRIPT_NAME -t <number>" 1>&2
              exit 1
           fi
           ;;

       g)  USE_GZ=1
           ;;

       h)  show_help $SCRIPT_NAME
           exit 0
           ;;

       :)  echo "$SCRIPT_NAME: Must supply an argument for -$OPTARG." >&2
           show_help $SCRIPT_NAME
           exit 1
           ;;

       \?) echo "Invalid option -$OPTARG ignored." >&2
           show_help $SCRIPT_NAME
           exit 1
           ;;
  esac
done

# check arguments
if [ -n "$NCDU_DIR" ]; then
   tmp_output_name=`echo -n "$NCDU_DIR" | tr '/[:space:]' '_'`
   NCDU_RES="$RES_DIR/ncdu.${tmp_output_name}.gz"
   USE_GZ=1
   ncdu -0xo- "$NCDU_DIR" | gzip >"${NCDU_RES}"
fi

if [ -z "$NCDU_RES" ]; then
   echo -e "$SCRIPT_NAME: invalid input, need a '-f' or '-d' option at least.\n" 1>&2
   show_help $SCRIPT_NAME
   exit 1
fi

if [ ! -f "$NCDU_RES" ]; then
   echo -e "\033[7m$NCDU_RES\033[0m should be a file be generated by ncdu -o." 1>&2
   show_help $SCRIPT_NAME
   exit 1
else
   # try to detect file type
   if [ "`file "$NCDU_RES" | cut -d" " -f2`" = "gzip" ]; then
      USE_GZ=1
   fi
fi

# create a temp tmux session
[ $USE_GZ -eq 1 ] && NCDU_READ_CMD="zcat $NCDU_RES | ncdu -f-" || NCDU_READ_CMD="ncdu -f $NCDU_RES"
tmux new -s ${TMUX_SESSION_NAME} -d "$NCDU_READ_CMD"

# create temp file to save result
TMP_RES=$(mktemp ${ALL_RES}.XXXXXXXXXX) || { echo "Failed to create temp file"; exit 1; }
HEADER=$(mktemp ${ALL_RES}.XXXXXXXXXX) || { echo "Failed to create temp header"; exit 1; }
FOOTER=$(mktemp ${ALL_RES}.XXXXXXXXXX) || { echo "Failed to create temp footer"; exit 1; }

# write header and footer
echo -e "Please check details with command:\n$NCDU_READ_CMD\n\n" > ${HEADER}
#df -h >> ${HEADER} && echo -e "\n\n" >> ${HEADER}
echo -e "This result was generated by command:\n$SCRIPT_NAME $@\n" > ${FOOTER}

# wait ncdu to draw
wait_draw "$TMP_RES"

# switch ncdu view
tmux send-keys -t ${TMUX_SESSION_NAME} 'g'
tmux send-keys -t ${TMUX_SESSION_NAME} 'g'

# save overall result
tmux_capture "${TMP_RES}"
sed -i -e '1d' -e '/^\S*$/d' ${TMP_RES}

ROW_MAX=$((`cat ${TMP_RES} | wc -l`))
if [ $ROW_MAX -lt 3 ]; then
   echo "$0: Invalid ncdu result." 1>&2
   exit 1
fi

# ignore head line and end line
ROW_MAX=$(($ROW_MAX - 2))
echo -e '\n' >> ${TMP_RES}

[ $ROW_MAX -lt $TOP_MAX ] && ROW_LIMIT=$ROW_MAX || ROW_LIMIT=$TOP_MAX

for ((i=1; i<=$ROW_LIMIT; i++)); do

    ROW="`sed -n "$((${i} + 1))p" ${TMP_RES}`"
    # is the entry a directory or file ?
    is_directory "$ROW"
    if [ $? -eq 1 ]; then
       # $ROW is dir
       tmux send-keys -t ${TMUX_SESSION_NAME} 'Enter'
       sleep ${WAIT_INTERVAL}
       tmux_capture "${TMP_RES}.${i}"
       sed -i -e '1d' -e '/^\S*$/d' ${TMP_RES}.${i}
       # cursor back to upper layer
       tmux send-keys -t ${TMUX_SESSION_NAME} 'Left'
    else
       # $ROW is file
       echo `head -1 $TMP_RES` > ${TMP_RES}.${i}
       echo "$ROW" >> ${TMP_RES}.${i}
    fi

    echo -e '\n\n' >> ${TMP_RES}.${i}
    # move cursor to next entry
    tmux send-keys -t ${TMUX_SESSION_NAME} 'Down'

done

# exit temp tmux session
tmux kill-session -t ${TMUX_SESSION_NAME}

echo -e "\n--- TOP ${ROW_LIMIT} ---\n" >> ${TMP_RES}

# combine result and mail
if [ -n "${MAIL_TO}" ]; then
   [ -n "${NCDU_DIR}" ] && tmp_ncdu_target="${NCDU_DIR}" || tmp_ncdu_target="${NCDU_RES}"
   cat "${HEADER}" ${TMP_RES}* "${FOOTER}" | mail ${MAIL_TO} -s "[lpd-eng-ccm][DISK USAGE] ${tmp_ncdu_target}"
else
   cat "${HEADER}" ${TMP_RES}* "${FOOTER}"
fi

# clean up
rm -f ${HEADER} ${TMP_RES}* ${FOOTER}