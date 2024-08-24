NAME           := kernel
SRCS           := framework/main.c $(shell find -L ./src/ -name "*.c")
INC_PATH       := include/ framework/

export AM_HOME := $(PWD)/../abstract-machine
ifeq ($(ARCH),)
# export ARCH    := x86-qemu
# export ARCH    := x86_64-qemu
export ARCH    := native
endif

include $(AM_HOME)/Makefile
include ../oslabs.mk
image: git
