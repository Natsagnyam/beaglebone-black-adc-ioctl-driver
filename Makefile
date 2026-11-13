
# ============================================================
# Project: BeagleBone Black ADC Frequency Driver
#
# Targets:
#   make          - Build kernel module and userspace program
#   make all      - Build both components
#   make module   - Build only the kernel module
#   make app      - Build only the userspace application
#   make clean    - Remove generated build files
#   make help     - Display available commands
#
# Kernel module: freq_driver.ko
# Userspace app: test_ioctl
# ============================================================

# Name of the external kernel module object.
obj-m := freq_driver.o

# Directory containing the build files for the running kernel.
KDIR := /lib/modules/$(shell uname -r)/build

# Absolute path to the current project directory.
PROJECT_DIR := $(CURDIR)

# C compiler for the userspace application.
CC := gcc

# Enable common compiler warnings and optimization.
USER_CFLAGS := -Wall -Wextra -Wpedantic -O2

# Name of the userspace executable.
APP := test_ioctl

# Source file used to build the userspace executable.
APP_SRC := test_ioctl.c

# Shared header file containing ioctl command definitions.
IOCTL_HEADER := freq_ioctl.h

# Declare targets that are commands rather than real files.
.PHONY: all module app clean help

# Default target: build both components.
all: module app

# Build the Linux kernel module using the kernel build system.
module:
	$(MAKE) -C $(KDIR) M=$(PROJECT_DIR) modules

# Build the userspace application.
app: $(APP)

# Rebuild the application if its source or shared header changes.
$(APP): $(APP_SRC) $(IOCTL_HEADER)
	$(CC) $(USER_CFLAGS) -o $@ $(APP_SRC)

# Remove the generated kernel and userspace build files.
clean:
	$(MAKE) -C $(KDIR) M=$(PROJECT_DIR) clean
	rm -f $(APP)

# Display available build commands.
help:
	@echo "BBB Frequency Driver - Build Commands"
	@echo "-------------------------------------"
	@echo "make          Build kernel module and userspace app"
	@echo "make module   Build kernel module only"
	@echo "make app      Build userspace application only"
	@echo "make clean    Remove generated build files"
	@echo "make help     Display this message"
