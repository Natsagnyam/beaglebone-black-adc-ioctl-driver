
#include <stdio.h>       // printf(), fopen(), fscanf(), fclose(), perror().
#include <stdlib.h>      // EXIT_SUCCESS and EXIT_FAILURE.
#include <stdint.h>      // Defines uint32_t (unsigned 32-bit integer).
#include <fcntl.h>       // Provides open() and O_RDWR.
#include <unistd.h>      // Provides close() and sleep().
#include <sys/ioctl.h>   // Provides the ioctl() function.

#include "freq_ioctl.h"  // Defines our FREQ_IOCTL_SET_ADC and FREQ_IOCTL_GET_FREQ commands.

// Path to our custom Linux character device.
#define DEVICE_PATH "/dev/bbb_freq"

// Path to ADC channel 0, provided by the Linux IIO driver.
#define ADC_PATH "/sys/bus/iio/devices/iio:device0/in_voltage0_raw"

int main(void)
{
    // File descriptor for communicating with our kernel driver.
    int fd;

    // File pointer for reading the ADC sysfs file.
    FILE *adc_file;

    // Temporary signed integer for the raw ADC reading.
    int raw;

    // Unsigned 32-bit value sent to the kernel driver.
    uint32_t adc_value;

    // Unsigned 32-bit frequency returned by the kernel.
    uint32_t frequency_mhz;

    // Frequency converted to floating-point hertz.
    double frequency_hz;

    // Open our custom character device once, before the loop.
    fd = open(DEVICE_PATH, O_RDWR);

    // Check whether opening the device was successful.
    if (fd == -1)
    {
        // Print the operating system error message.
        perror("Cannot open /dev/bbb_freq");

        // Exit if the device cannot be opened.
        return EXIT_FAILURE;
    }

    // Tell the user that the device is ready.
    printf("BBB ADC0 to ioctl frequency monitor\n");

    // Display a heading for the readings.
    printf("-----------------------------------\n");

    // Continuously acquire ADC samples.
    while (1)
    {
        // Open the Linux IIO ADC channel 0 file for reading.
        adc_file = fopen(ADC_PATH, "r");

        // Check whether the ADC file opened successfully.
        if (adc_file == NULL)
        {
            // Display an error if the ADC file is unavailable.
            perror("Cannot open ADC0");

            // Leave the loop so we can close the character device.
            break;
        }

        // Read the ADC count from the sysfs file.
        if (fscanf(adc_file, "%d", &raw) != 1)
        {
            // Display an error if no valid integer was read.
            fprintf(stderr, "Cannot read ADC0\n");

            // Close the ADC file before leaving.
            fclose(adc_file);

            // Stop the monitoring loop.
            break;
        }

        // Close the ADC file because the sample has been obtained.
        fclose(adc_file);

        // Verify that the ADC count is within the 12-bit range.
        if (raw < 0 || raw > 4095)
        {
            // Report an unexpected ADC value.
            fprintf(stderr, "ADC out of range: %d\n", raw);

            // Stop the monitoring loop.
            break;
        }

        // Convert the signed ADC integer into an unsigned integer.
        adc_value = (uint32_t)raw;

        // Send the live ADC count to our custom kernel driver.
        if (ioctl(fd, FREQ_IOCTL_SET_ADC, &adc_value) == -1)
        {
            // Display the cause of the ioctl error.
            perror("FREQ_IOCTL_SET_ADC ioctl failed");

            // Stop if the driver cannot accept the ADC reading.
            break;
        }

        // Initialize the output variable before using it.
        frequency_mhz = 0;

        // Request the frequency calculated by our kernel driver.
        if (ioctl(fd, FREQ_IOCTL_GET_FREQ, &frequency_mhz) == -1)
        {
            // Display the cause of the ioctl error.
            perror("FREQ_IOCTL_GET_FREQ ioctl failed");

            // Stop if the driver cannot return the frequency.
            break;
        }

        // Convert millihertz into hertz.
        frequency_hz = frequency_mhz / 1000.0;

        // Print the ADC value and frequency on one terminal line.
        printf("ADC0 = %4u | Frequency = %6.3f Hz\n",
               adc_value,
               frequency_hz);

        // Wait 500 milliseconds before sampling again.
        // This produces two readings per second.
        usleep(500000);
    }

    // Close our custom character device after leaving the loop.
    close(fd);

    // Report that the program stopped due to an error.
    return EXIT_FAILURE;
}
