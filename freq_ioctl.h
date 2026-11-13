
#ifndef FREQ_IOCTL_H
#define FREQ_IOCTL_H

#include <linux/ioctl.h>    // Linux ioctl command encoding macros.
#include <linux/types.h>    // Fixed-width integer types.

/*
 * ioctl command family identifier.
 *
 * All ioctl commands belonging to this driver use 'F'.
 */
#define FREQ_IOCTL_MAGIC                   'F'

/*
 * ioctl command numbers.
 *
 * Each operation has a unique command number.
 */
#define FREQ_IOCTL_CMD_NUMBER_FREQ_IOCTL_SET_ADC       1
#define FREQ_IOCTL_CMD_NUMBER_FREQ_IOCTL_GET_FREQ      2

/*
 * FREQ_IOCTL_SET_ADC:
 * Transfer a 32-bit ADC count from userspace to kernel space.
 *
 * Valid ADC range: 0 to 4095.
 * our driver reads adc from user space
 */
#define FREQ_IOCTL_SET_ADC \
    _IOW(FREQ_IOCTL_MAGIC, \
         FREQ_IOCTL_CMD_NUMBER_FREQ_IOCTL_SET_ADC, \
         __u32)

/*
 * FREQ_IOCTL_GET_FREQ:
 * Transfer a 32-bit frequency value from kernel space to userspace.
 *
 * Frequency unit: millihertz (mHz).
 * Example: 55000 mHz = 55.000 Hz.
 * our driver generates Hz bu using adc
 */
#define FREQ_IOCTL_GET_FREQ \
    _IOR(FREQ_IOCTL_MAGIC, \
         FREQ_IOCTL_CMD_NUMBER_FREQ_IOCTL_GET_FREQ, \
         __u32)

#endif /* FREQ_IOCTL_H */
