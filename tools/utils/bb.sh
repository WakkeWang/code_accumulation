#!/bin/sh

flag=arm64

if [ "$flag" == "ppc" ];then
	source /data/toolchain/setenv/setenv_ppc64e5500_2.0
	make $1 ARCH=ppc CROSS_COMPILE=powerpc64-fsl-linux-
fi

if [ "$flag" == "arm64" ];then
	source /data/toolchain/setenv/setenv_linaro_7.2.1
	make $1 ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu-
fi
