#!/bin/sh

case "${ARCH}" in
    "x86_64")
	export ARCH_CFLAGS="-mcmodel=kernel"
    ;;
esac
