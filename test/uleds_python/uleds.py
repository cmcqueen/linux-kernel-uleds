#!/usr/bin/python3

import struct

def run_led(name, max_brightness=255):
    with open('/dev/uleds', 'r+b', buffering=0) as f:
        name_struct = struct.pack('@64sI', name.encode('utf-8'), max_brightness)
        print(type(name_struct))
        f.write(name_struct)
        brightness_size = len(struct.pack('@I', 0))
        previous_brightness = None
        while True:
            led_brightness_data = f.read(brightness_size)
            if led_brightness_data:
                led_brightness, = struct.unpack('@I', led_brightness_data)
                if led_brightness != previous_brightness:
                    print('Brightness {}'.format(led_brightness))
                    previous_brightness = led_brightness

def main():
    run_led('uleds::test')

if __name__ == '__main__':
    main()
