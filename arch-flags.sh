#!/bin/sh

case "${ARCH}" in
    "x86_64")
	export ARCH_CFLAGS="-mcmodel=kernel"
	export ARCH_LDFLAGS="-z max-page-size=0x1000"
    ;;
esac
