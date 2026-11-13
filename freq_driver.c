
#include <linux/module.h>      // Support Linux kernel modules.
#include <linux/init.h>        // Module initialization and cleanup.
#include <linux/fs.h>          // Character device file operations.
#include <linux/miscdevice.h>  // Simplified device registration.
#include <linux/uaccess.h>     // Copy data between kernel and userspace.
#include <linux/mutex.h>       // Protect shared variables.
#include <linux/errno.h>       // Linux error codes.

#include "freq_ioctl.h"        // Our ioctl command definitions.

// Store the latest raw ADC value sent from userspace.
static __u32 adc_raw = 0;

// Create a mutex to protect access to adc_raw.
static DEFINE_MUTEX(adc_lock);

// Handle ioctl requests from userspace.
static long freq_ioctl(struct file *file,
                       unsigned int cmd,
                       unsigned long arg)
{
    __u32 value;               // Store data received from userspace.
    __u32 frequency_mhz;       // Calculated frequency in millihertz.

    // Select the operation requested by the application.
    switch (cmd)
    {
        case FREQ_IOCTL_SET_ADC:

            // Copy the ADC value from userspace to the kernel.
            if (copy_from_user(&value,
                               (void __user *)arg,
                               sizeof(value)))
            {
                // Report an invalid userspace memory access.
                return -EFAULT;
            }

            // The BBB ADC has 12-bit resolution.
            // Therefore its valid raw range is 0–4095.
            if (value > 4095)
            {
                // Reject readings outside the ADC range.
                return -EINVAL;
            }

            // Protect the shared ADC variable.
            mutex_lock(&adc_lock);

            // Save the new reading.
            adc_raw = value;

            // Release the mutex.
            mutex_unlock(&adc_lock);

            // Tell the application the command succeeded.
            return 0;

        case FREQ_IOCTL_GET_FREQ:

            // Protect the value while reading it.
            mutex_lock(&adc_lock);

            // Copy the ADC reading into a local variable.
            value = adc_raw;

            // Release the mutex.
            mutex_unlock(&adc_lock);

            // Convert raw ADC count to millihertz.
            // ADC = 0    -> 45000 mHz = 45 Hz.
            // ADC = 2048 -> about 55002 mHz = 55 Hz.
            // ADC = 4095 -> 65000 mHz = 65 Hz.
            frequency_mhz = 45000 + (value * 20000) / 4095;

            // Send the calculated frequency back to userspace.
            if (copy_to_user((void __user *)arg,
                             &frequency_mhz,
                             sizeof(frequency_mhz)))
            {
                // Report a userspace memory copy error.
                return -EFAULT;
            }

            // Tell the application the command succeeded.
            return 0;

        default:

            // Reject commands that this driver does not support.
            return -ENOTTY;
    }
}

// List the operations supported by our character device.
static const struct file_operations freq_fops = {
    .owner = THIS_MODULE,           // Kernel module owning the device.
    .unlocked_ioctl = freq_ioctl,   // Our ioctl handling function.
};

// Define our character device.
static struct miscdevice freq_device = {
    .minor = MISC_DYNAMIC_MINOR,    // Request an available minor number.
    .name = "bbb_freq",            // Device appears as /dev/bbb_freq.
    .fops = &freq_fops,            // Assign our supported operations.
    .mode = 0600,                  // Allow only root to access the device.
};

// Called when Linux loads our driver.
static int __init freq_init(void)
{
    int result;                     // Store registration status.

    // Register the character device with the kernel log.
    result = misc_register(&freq_device);

    // Check whether registration failed.
    if (result)
    {
        pr_err("bbb_freq: registration failed\n");
        return result;
    }

    // Write a success message into the kernel log.
    pr_info("bbb_freq: driver loaded\n");

    return 0;                       // Initialization successful.
}

// Called when Linux unloads our driver.
static void __exit freq_exit(void)
{
    // Unregister the character device.
    misc_deregister(&freq_device);

    // Write an informational message into the kernel log.
    pr_info("bbb_freq: driver unloaded\n");
}

// Connect initialization and exit functions to the module.
module_init(freq_init);
module_exit(freq_exit);

// Module information.
MODULE_LICENSE("GPL");
MODULE_AUTHOR("BBB Beginner Project");
MODULE_DESCRIPTION("BeagleBone ADC frequency ioctl driver");
MODULE_VERSION("1.0");
