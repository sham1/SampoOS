#!/bin/sh

for arch in x86_64
do
    export ARCH="${arch}"
    . ./arch-flags.sh
    make -C Kernel
done
