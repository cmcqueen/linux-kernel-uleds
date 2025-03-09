/* SPDX-License-Identifier: GPL-2.0+ WITH Linux-syscall-note */
/*
 * Userspace driver support for the LED subsystem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _UAPI__ULEDS_H_
#define _UAPI__ULEDS_H_

#define LED_MAX_NAME_SIZE		64
#define ULEDS_TRIGGER_MAX_NAME_SIZE	64

struct uleds_user_dev {
	char name[LED_MAX_NAME_SIZE];
	int max_brightness;
};

struct uleds_user_trigger {
	char name[ULEDS_TRIGGER_MAX_NAME_SIZE];
};


/* ioctl commands */

#define ULEDS_IOC_MAGIC		'l'

/*
 * Initial setup.
 * E.g.:
 *	int retval;
 *	struct uleds_user_dev dev_setup = { "mainboard:green:battery", 255 };
 *	retval = ioctl(fd, ULEDS_IOC_DEV_SETUP, &dev_setup);
 */
#define ULEDS_IOC_DEV_SETUP		_IOW(ULEDS_IOC_MAGIC, 0x01, struct uleds_user_dev)

/*
 * Set the default trigger.
 * E.g.:
 *	int retval;
 *	struct uleds_user_trigger default_trigger = { "battery" };
 *	retval = ioctl(fd, ULEDS_IOC_SET_DEFAULT_TRIGGER, &default_trigger);
 */
#define ULEDS_IOC_SET_DEFAULT_TRIGGER	_IOW(ULEDS_IOC_MAGIC, 0x02, struct uleds_user_trigger)

#endif /* _UAPI__ULEDS_H_ */
