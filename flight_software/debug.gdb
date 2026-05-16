# Run debug with gdb-multiarch foo.elf -x debug.gdb

target extended-remote localhost:3333
monitor halt
load
monitor reset halt
break main
continue