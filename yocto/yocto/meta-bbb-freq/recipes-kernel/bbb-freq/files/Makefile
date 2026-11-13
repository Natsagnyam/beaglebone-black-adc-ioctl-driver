# This Makefile is only for the Yocto external-kernel-module build.
obj-m := freq_driver.o
KERNEL_SRC ?= /lib/modules/$(shell uname -r)/build
MODULE_DIR := $(shell pwd)

all:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_DIR) modules

modules_install:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_DIR) modules_install

clean:
	$(MAKE) -C $(KERNEL_SRC) M=$(MODULE_DIR) clean
