#!/bin/sh

for arch in x86_64
do
    make -C Kernel ARCH="${arch}"
done
