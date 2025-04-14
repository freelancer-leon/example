#!/usr/bin/env bash
#set -x

# 1) Need gdbinit file as below
#   set pagination off
#   set logging overwrite on
#   set logging file map.log
#   set logging on
#   i proc mappings
#   set logging off
#   shell ./gdb_dump_vma.sh
#   set logging file gdb.log
#   set logging on
#   show logging
#   i proc mappings
#   source dump_vma.gdb
#
# 2) put this script and gdbinit in the same folder
#
# 3) Run file in gdb
# (gdb) source gdbinit
#
# Then the dump all VMAs into gdb.log

function gen_dump_vma_commands()
{
    local input="$1"
	local output="$2"
    local start
    local size

	# Remove the heading 3 lines as below from input file
    # Mapped address spaces:
    #
    #      Start Addr           End Addr       Size     Offset objfile
    sed -i.bak "1,3d" $input
	# Empty the output file
	echo -n "" > $output

    while read LINE; do
          start="$(echo ${LINE} | awk '{ print $1 }')"
          size="$(echo ${LINE} | awk '{ print $3 }')"

		  if [[ -n "$start" && -n "$size" ]]; then
		     printf "x /%dxg 0x%x\n" $(($size>>3)) $start >> $output
		  fi
    done <$input
}

function main()
{
    gen_dump_vma_commands map.log dump_vma.gdb
}

main "$*"
