#!/usr/bin/env bash

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 [start|attach] [binary|elf]"
    exit
fi


if [[ $1 == "start" ]]; then
    (cd build && make)
    echo "Starting GDB server on port 1234"
    socat  \
          TCP-LISTEN:1234,reuseaddr \
          EXEC:"./build/host -d $2"
elif [[ $1 == "attach" ]]; then
    echo "Conneting to GDB server on port 1234"
    riscv32-elf-gdb -ex 'target remote :1234' -ex 'layout src' $2
else
    echo "Usage: $0 [start|attach] [binary|elf]"
fi


