NAME := kernel
SRCS := $(shell find . -name "*.c")
export ARCH := x86-qemu
include $(AM_HOME)/Makefile

# debug:
# 	qemu-system-x86_64 -s -S build/kernel-x86_64-qemu

debug:
	qemu-system-i386 -s -S -machine accel=tcg -smp "1,sockets=1" \
		-drive format=raw,file=build/kernel-x86-qemu &
	gdb -x /home/mx/os-workbench/else/debug-bootloader/debug.py
