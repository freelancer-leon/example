#!/usr/bin/env bash
#set -x

function print_commit_message_templete()
{
  echo -e "--------------------------------------------
<scope>: <summary>\n
Issue: <issue id>\n
[some messages]\n
Signed-off-by: Somebody <somebody@email.address>
--------------------------------------------\n"
}

function get_option_value()
{
  local options="$1"
  local option="$2"
  local b=${options##*${option} }
  local a=${b%% *}
  echo $a
}

# Check summary format, should be
#   <scope>: <summary>
# Note:
#   There must a space after the first ":"
# Returns:
#   0: check failed
#   1: check passed
#   2: check skipped
function check_summary()
{
  local summary="$1"
  # Get the string before the first space
  local scope="${summary%% *}"
  # Skip "Merge" or "Revert" commits
  [[ "$scope" == "Revert" || "$scope" == "Merge" ]] && return 2

  local last_char="${scope: -1}"
  [ "$last_char" != ":" ] && return 0

  return 1
}

# Check issue format, should be
#   Issue: <issue id>
# or
#   issue: <issue id>
# Note:
#   There must be a space after the first ":"
# Returns:
#   0: check failed
#   1: check passed
function check_issue_id()
{
  local third_line="$1"
  # Get the string before the first space
  local issue_str="${third_line%% *}"
  [[ "$issue_str" != "Issue:" && "$issue_str" != "issue:" ]] && return 0
  local id="${third_line#* }"
  [[ "$id" == "${issue_str}" || -z "$id" ]] && return 0
  return 1
}

# Check signature
#
# At least one 'Signed-off-by:' should be in commit message.
#
# Returns:
#   0: check failed
#   1: check passed
function check_signature()
{
  local commit_message="$1"
  if ! echo "$commit_message" | grep 'Signed-off-by:' >/dev/null ; then
     return 0
  fi
  return 1
}

# Returns:
#   0: check failed
#   1: check passed
function check_commit()
{
  local commit="$1"
  local commit_message="$(git show -s --format=%B $commit)"

  local summary=$(echo "$commit_message" | head -1)
  # Check the first line of commit message, that is, summary
  check_summary "$summary"
  local res=$?
  if [ $res -eq 2 ]; then
     echo "INFO: skip check [$commit $summary]"
     return 1
  elif [ $res -eq 0 ]; then
     echo -e "ERROR: Invalid summary [$commit $summary]
       Expected summary format should be:
       <scope>: <summary>\n"
     return 0
  fi

  # A convensional commit message needs at leat 5 lines.
  local commit_line_number="$(echo "$commit_message" | wc -l)"
  if [ $commit_line_number -lt 5 ]; then
     echo -e "ERROR: unconventional commit message [$commit $summary],
       A conventional commit message should be:\n"
     print_commit_message_templete
     return 0
  fi

  # Check the second line of commit message, it should be a blank line
  local second_line=$(echo "$commit_message" | sed -n "2p")
  if [ ! -z "$second_line" ]; then
     echo -e "ERROR: the second line is invalid [$commit $summary]
       Expect an empty line for the second line of commit message.\n"
     return 0
  fi

  # Check the third line of commit message, it should has issue id
  local third_line=$(echo "$commit_message" | sed -n "3p")
  check_issue_id "$third_line"
  res=$?
  if [ $res -eq 0 ]; then
     echo -e "ERROR: the third line is invalid [$commit $summary]\n
       Now the third line is:
       $third_line\n
       Expect an issue id at this line, the format should be:
       Issue: <issue id>\n"
     return 0
  fi

  # Check signature
  check_signature "$commit_message"
  res=$?
  if [ $res -eq 0 ]; then
     echo -e "ERROR: Signature check failed.
       Cannot find at least one 'Signed-off-by:' in message body [$commit $summary]\n"
     return 0
  fi

  return 1
}

function main()
{
  local args="$*"
  local oldrev=$(get_option_value "$args" "--oldrev")
  local newrev=$(get_option_value "$args" "--newrev")
  local commit_list=$(git log ${oldrev}..${newrev} --pretty=format:"%h")

  local res=0
  local err=0
  for c in $commit_list; do
      check_commit $c
      # Don't break if error, check all commits at a time,
      # so that the submitter can learns all the errors.
      res=$?
      [ $res -eq 0 ] && err=$(($err + 1))
  done
  local s=""
  [ $err -gt 1 ] && s="s"
  [ $err -gt 0 ] && { echo "ERROR: detect ${err} unconventional commit message${s}"; exit -1; }
}

main "$*"
