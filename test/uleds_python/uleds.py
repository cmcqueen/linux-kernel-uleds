#!/usr/bin/python3

import selectors
import struct

CHAR_DEV_NAME = '/dev/uleds'
MAX_BRIGHTNESS=255
SETUP_FORMAT = '@64sI'
BRIGHTNESS_FORMAT = '@I'
BRIGHTNESS_SIZE = struct.calcsize(BRIGHTNESS_FORMAT)

led_names = ('uleds::test0', 'uleds::test1')

def uleds_register(names, max_brightness=MAX_BRIGHTNESS):
    uleds_f = []
    for name in names:
        f = open(CHAR_DEV_NAME, 'r+b', buffering=0)
        name_struct = struct.pack(SETUP_FORMAT, name.encode('utf-8'), max_brightness)
        f.write(name_struct)
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
