// SPDX-License-Identifier: GPL-2.0
/*
 * uledmon.c
 *
 * This program creates a new userspace LED class device and monitors it. A
 * timestamp and brightness value is printed each time the brightness changes.
 *
 * Usage: uledmon <device-name> [default-trigger]
 *
 * <device-name> is the name of the LED class device to be created. Pressing
 * CTRL+C will exit.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <linux/uleds.h>

int main(int argc, char const *argv[])
{
	struct uleds_user_dev uleds_dev;
	struct uleds_user_trigger default_trigger = {};
	int fd, ret;
	int brightness;
	struct timespec ts;

	if (argc < 2) {
		fprintf(stderr, "Usage:\n    uledmon <device-name> [default-trigger]\n");
		return 1;
	}
	if (argc >= 3) {
		strncpy(default_trigger.name, argv[2], ULEDS_TRIGGER_MAX_NAME_SIZE);
	}

	strncpy(uleds_dev.name, argv[1], LED_MAX_NAME_SIZE);
	uleds_dev.max_brightness = 100;

	fd = open("/dev/uleds", O_RDWR);
	if (fd == -1) {
		perror("Failed to open /dev/uleds");
		return 1;
	}

#if 0
	// Setup by write.
	ret = write(fd, &uleds_dev, sizeof(uleds_dev));
#else
	// Setup by ioctl.
	ret = ioctl(fd, ULEDS_IOC_DEV_SETUP, &uleds_dev);
#endif
	if (ret == -1) {
		perror("Failed to set up LED");
		close(fd);
		return 1;
	}

	if (default_trigger.name[0]) {
		// Set a default trigger.
		ret = ioctl(fd, ULEDS_IOC_SET_DEFAULT_TRIGGER, &default_trigger);
		if (ret == -1) {
			perror("Failed to set default trigger");
			close(fd);
			return 1;
		}
	}

	while (1) {
		ret = read(fd, &brightness, sizeof(brightness));
		if (ret == -1) {
			perror("Failed to read from /dev/uleds");
			close(fd);
			return 1;
		}
		clock_gettime(CLOCK_MONOTONIC, &ts);
		printf("[%ld.%09ld] %u\n", ts.tv_sec, ts.tv_nsec, brightness);
	}

	close(fd);

	return 0;
}
