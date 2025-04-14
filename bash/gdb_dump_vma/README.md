These 2 files can be used to dump the conttent of VMAs from coredump

1. `gdbinit` file will do these in gdb:
   * set logging file as `map.log`
   * dump vma list into `map.log`
   * run shell script `./gdb_dump_vma.sh` to generate dump VMAs commands `dump_vma.gdb`
   * set logging file as `gdb.log`
   * source `dump_vma.gdb` to run dumping VMAs gdb commands

2. put this script and gdbinit in the same folder

3. Run file in gdb
```sh
(gdb) source gdbinit
```

Then dump all VMAs into `gdb.log`
