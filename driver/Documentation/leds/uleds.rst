==============
Userspace LEDs
==============

The uleds driver supports userspace LEDs. This can be useful for testing
triggers and can also be used to implement virtual LEDs.


Usage
=====

When the driver is loaded, a character device is created at /dev/uleds. To
create a new LED class device, open /dev/uleds and write a uleds_user_dev
structure to it (found in kernel public header file linux/uleds.h)::

    #define LED_MAX_NAME_SIZE 64

    struct uleds_user_dev {
	char name[LED_MAX_NAME_SIZE];
	int max_brightness;
    };

A new LED class device will be created with the name given. The name can be
any valid sysfs device node name, but consider using the LED class naming
convention of "devicename:color:function".

Alternatively, setup can be done with an ioctl call, passing a pointer to
the structure.

There is also an ioctl call to configure a default trigger for the LED, by
passing a pointer to a structure containing the name of a trigger.

The current brightness is found by reading an int value from the character
device. Reading will block until the brightness changes. The device node can
also be polled to notify when the brightness value changes.

The LED class device will be removed when the open file handle to /dev/uleds
is closed.

Multiple LED class devices are created by opening additional file handles to
/dev/uleds.

See tools/leds/uledmon.c for an example userspace program.
