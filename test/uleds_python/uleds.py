#!/usr/bin/python3

from enum import IntEnum
import fcntl
import selectors
import struct

# 3rd-party packages
from ioctl_opt import IOC, IOC_WRITE

led_names = ('uleds::test0', 'uleds::test1')

CHAR_DEV_NAME = '/dev/uleds'
MAX_BRIGHTNESS=255
SETUP_FORMAT = '@64sI'
TRIGGER_FORMAT = '@64s'
BRIGHTNESS_FORMAT = '@I'
BRIGHTNESS_SIZE = struct.calcsize(BRIGHTNESS_FORMAT)

class EnumMissingUnknown():
    @classmethod
    def _missing_(cls, value):
        return cls.Unknown

class IOCTL(EnumMissingUnknown, IntEnum):
    Unknown        = -1
    ULEDS_IOC_DEV_SETUP             = IOC(IOC_WRITE, ord('l'), 0x01, 68)
    ULEDS_IOC_SET_DEFAULT_TRIGGER   = IOC(IOC_WRITE, ord('l'), 0x02, 64)

def uleds_register(names, max_brightness=MAX_BRIGHTNESS):
    uleds_f = []
    for name in names:
        f = open(CHAR_DEV_NAME, 'r+b', buffering=0)

        # Set up name and max brightness.
        name_struct = struct.pack(SETUP_FORMAT, name.encode('utf-8'), max_brightness)
        if False:
            f.write(name_struct)
        else:
            fcntl.ioctl(f, IOCTL.ULEDS_IOC_DEV_SETUP, name_struct)

        # Set default trigger
        default_trigger_struct = struct.pack(TRIGGER_FORMAT, 'utest0'.encode('utf-8'))
        fcntl.ioctl(f, IOCTL.ULEDS_IOC_SET_DEFAULT_TRIGGER, default_trigger_struct)

        uleds_f.append((name, f))
    return uleds_f

def uleds_poll(uleds_f):
    sel = selectors.DefaultSelector()
    for name, f in uleds_f:
        sel.register(f, selectors.EVENT_READ, name)
    try:
        while True:
            events = sel.select()
            for key, _mask in events:
                name = key.data
                led_brightness_data = key.fileobj.read(BRIGHTNESS_SIZE)
                if led_brightness_data:
                    led_brightness, = struct.unpack(BRIGHTNESS_FORMAT, led_brightness_data)
                    print('{}: brightness {}'.format(name, led_brightness))
    finally:
        for name, f in uleds_f:
            sel.unregister(f)

def main():
    uleds_f = uleds_register(led_names)
    uleds_poll(uleds_f)
    for _name, f in uleds_f:
        f.close()

if __name__ == '__main__':
    main()
