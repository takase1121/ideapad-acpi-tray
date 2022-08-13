#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>

/**
 * This program repeatedly reads from the file and
 * checks if the values are different. This controls
 * the timeout between each read to reduce CPU.
 * If this is too high, it might affect the systray
 * response time.
 */
static const struct timespec WAIT_TIMEOUT = {
	/** seconds */
	.tv_sec = 0,
	/** nanoseconds - 80ms by default */
	.tv_nsec = 80000000L
};

/**
 * The default systray icon. This will be replaced as
 * the status is read.
 * This can be an absolute path to an image.
 */
#define DEFAULT_ICON "face-suprise"
/**
 * The systray icon when battery conservation is enabled.
 * This can be an absolute path to an image.
 */
#define CONSERVATION_ENABLED_ICON "face-monkey"
/**
 * The systray icon when battery conservation is disabled.
 * This can be an absolute path to an image.
 */
#define CONSERVATION_DISABLED_ICON "face-tired"

/** 
 * The path to the ideapad_acpi driver.
 * Its unlikely that you'll need to modify this.
 */
const char IDEAPAD_ACPI_PATH[] = "/sys/bus/platform/drivers/ideapad_acpi";
/**
 * The device ID prefix.
 * Its unlikely that you'll need to modify this.
 */
const char IDEAPAD_DEVICE_PREFIX[] = "VPC";

/**
 * If defined, the program will not attempt to search IDEAPAD_ACPI_PATH.
 * It'll read the device ID from command line arguments.
 */
// #define DO_NOT_SEARCH

#endif