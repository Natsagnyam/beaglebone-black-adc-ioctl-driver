SUMMARY = "BBB educational ADC count to frequency ioctl kernel module"
DESCRIPTION = "Out-of-tree miscellaneous character device driver for /dev/bbb_freq"
LICENSE = "GPL-2.0-only"
# Replace this placeholder with the actual checksum from your LICENSE file.
LIC_FILES_CHKSUM = "file://LICENSE;md5=REPLACE_WITH_REAL_MD5"

inherit module

SRC_URI = "file://freq_driver.c \
           file://freq_ioctl.h \
           file://Makefile \
           file://LICENSE"

S = "${WORKDIR}"

# Install a modules-load.d entry so the module loads when the board boots.
KERNEL_MODULE_AUTOLOAD += "freq_driver"
