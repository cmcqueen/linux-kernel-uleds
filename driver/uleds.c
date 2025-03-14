// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Userspace driver for the LED subsystem
 *
 * Copyright (C) 2016 David Lechner <david@lechnology.com>
 *
 * Based on uinput.c: Aristeu Sergio Rozanski Filho <aris@cathedrallabs.org>
 */
#include <linux/ctype.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include <uapi/linux/uleds.h>

#define ULEDS_NAME	"uleds"

enum uleds_state {
	ULEDS_STATE_UNKNOWN,
	ULEDS_STATE_REGISTERED,
};

struct uleds_device {
	struct uleds_user_dev		user_dev;
	struct uleds_user_trigger	default_trigger;
	struct led_classdev		led_cdev;
	struct mutex			mutex;
	enum uleds_state		state;
	wait_queue_head_t		waitq;
	int				brightness;
	bool				new_data;
};

static struct miscdevice uleds_misc;

static void uleds_brightness_set(struct led_classdev *led_cdev,
				 enum led_brightness brightness)
{
	struct uleds_device *udev = container_of(led_cdev, struct uleds_device,
						 led_cdev);

	if (udev->brightness != brightness) {
		udev->brightness = brightness;
		udev->new_data = true;
		wake_up_interruptible(&udev->waitq);
	}
}

static int uleds_open(struct inode *inode, struct file *file)
{
	struct uleds_device *udev;

	udev = kzalloc(sizeof(*udev), GFP_KERNEL);
	if (!udev)
		return -ENOMEM;

	udev->led_cdev.name = udev->user_dev.name;
	udev->led_cdev.brightness_set = uleds_brightness_set;

	mutex_init(&udev->mutex);
	init_waitqueue_head(&udev->waitq);
	udev->state = ULEDS_STATE_UNKNOWN;

	file->private_data = udev;
	stream_open(inode, file);

	return 0;
}

static bool is_led_name_valid(const char *name)
{
	return ((name[0] != '\0') &&
		(strcmp(name, ".") != 0) &&
		(strcmp(name, "..") != 0) &&
		(strchr(name, '/') == NULL));
}

static int dev_setup(struct uleds_device *udev, const char __user *buffer)
{
	int ret;

	ret = mutex_lock_interruptible(&udev->mutex);
	if (ret)
		return ret;

	if (udev->state == ULEDS_STATE_REGISTERED) {
		ret = -EBUSY;
		goto out;
	}

	if (copy_from_user(&udev->user_dev, buffer,
			   sizeof(struct uleds_user_dev))) {
		ret = -EFAULT;
		goto out;
	}

	if (!is_led_name_valid(udev->user_dev.name)) {
		ret = -EINVAL;
		goto out;
	}

	if (udev->user_dev.max_brightness <= 0) {
		ret = -EINVAL;
		goto out;
	}
	udev->led_cdev.max_brightness = udev->user_dev.max_brightness;

	ret = devm_led_classdev_register(uleds_misc.this_device,
					 &udev->led_cdev);
	if (ret < 0)
		goto out;

	udev->new_data = true;
	udev->state = ULEDS_STATE_REGISTERED;

out:
	mutex_unlock(&udev->mutex);

	return ret;
}

static ssize_t uleds_write(struct file *file, const char __user *buffer,
	size_t count, loff_t *ppos)
{
	struct uleds_device *udev = file->private_data;
	int ret;

	if (count == 0)
		return 0;
	if (count != sizeof(struct uleds_user_dev)) {
		return -EINVAL;
	}
	ret = dev_setup(udev, buffer);
	if (ret < 0)
		return ret;
	return count;
}

static ssize_t uleds_read(struct file *file, char __user *buffer, size_t count,
			  loff_t *ppos)
{
	struct uleds_device *udev = file->private_data;
	ssize_t retval;

	if (count < sizeof(udev->brightness))
		return 0;

	do {
		retval = mutex_lock_interruptible(&udev->mutex);
		if (retval)
			return retval;

		if (udev->state != ULEDS_STATE_REGISTERED) {
			retval = -ENODEV;
		} else if (!udev->new_data && (file->f_flags & O_NONBLOCK)) {
			retval = -EAGAIN;
		} else if (udev->new_data) {
			retval = copy_to_user(buffer, &udev->brightness,
					      sizeof(udev->brightness));
			udev->new_data = false;
			retval = sizeof(udev->brightness);
		}

		mutex_unlock(&udev->mutex);

		if (retval)
			break;

		if (!(file->f_flags & O_NONBLOCK))
			retval = wait_event_interruptible(udev->waitq,
					udev->new_data ||
					udev->state != ULEDS_STATE_REGISTERED);
	} while (retval == 0);

	return retval;
}

static __poll_t uleds_poll(struct file *file, poll_table *wait)
{
	struct uleds_device *udev = file->private_data;

	poll_wait(file, &udev->waitq, wait);

	if (udev->new_data)
		return EPOLLIN | EPOLLRDNORM;

	return 0;
}

/*
 * Trigger name validation: Allow only alphanumeric, hyphen or underscore.
 */
static bool is_trigger_name_valid(const char * name)
{
	size_t i;

	if (name[0] == '\0')
		return false;

	for (i = 0; i < TRIG_NAME_MAX; i++) {
		if (name[i] == '\0')
			break;
		if (!isalnum(name[i]) && name[i] != '-' && name[i] != '_')
			return false;
	}
	/* Length check and avoid any special names. */
	return ((i < TRIG_NAME_MAX) &&
		(strcmp(name, "default") != 0));
}

static long uleds_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct uleds_device *udev = file->private_data;
	struct uleds_user_trigger default_trigger;
	int retval = 0;

	switch (cmd) {
	case ULEDS_IOC_DEV_SETUP:
		retval = dev_setup(udev, (const char __user *)arg);
		break;

	case ULEDS_IOC_SET_DEFAULT_TRIGGER:
		retval = copy_from_user(&default_trigger,
			(struct uleds_user_trigger __user *)arg,
			sizeof(default_trigger));
		if (retval)
			return retval;
		retval = mutex_lock_interruptible(&udev->mutex);
		if (retval)
			return retval;
		if (default_trigger.name[0] == '\0') {
			udev->led_cdev.default_trigger = NULL;
		} else {
			if (!is_trigger_name_valid(default_trigger.name)) {
				mutex_unlock(&udev->mutex);
				return -EINVAL;
			}
			memcpy(&udev->default_trigger, &default_trigger, sizeof(udev->default_trigger));
			udev->led_cdev.default_trigger = udev->default_trigger.name;
		}
		if (udev->state == ULEDS_STATE_REGISTERED) {
			led_trigger_set_default(&udev->led_cdev);
		}
		mutex_unlock(&udev->mutex);
		break;

	default:
		retval = -ENOIOCTLCMD;
		break;
	}

	return retval;
}

static int uleds_release(struct inode *inode, struct file *file)
{
	struct uleds_device *udev = file->private_data;

	if (udev->state == ULEDS_STATE_REGISTERED) {
		udev->state = ULEDS_STATE_UNKNOWN;
		devm_led_classdev_unregister(uleds_misc.this_device,
					     &udev->led_cdev);
	}
	kfree(udev);

	return 0;
}

static const struct file_operations uleds_fops = {
	.owner		= THIS_MODULE,
	.open		= uleds_open,
	.release	= uleds_release,
	.read		= uleds_read,
	.write		= uleds_write,
	.poll		= uleds_poll,
	.unlocked_ioctl	= uleds_ioctl,
};

static struct miscdevice uleds_misc = {
	.fops		= &uleds_fops,
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= ULEDS_NAME,
};

module_misc_device(uleds_misc);

MODULE_AUTHOR("David Lechner <david@lechnology.com>");
MODULE_DESCRIPTION("Userspace driver for the LED subsystem");
MODULE_LICENSE("GPL");
