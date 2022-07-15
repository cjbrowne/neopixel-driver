#!/usr/bin/python3
import colorsys
import math
import time

# todo: how to make this work with windoze?
pxl = open("/dev/ttyACM0", "wb")

pxl.write(bytearray('r', 'utf8'))

res = 17

while 1:
    for i in range(res):
        (r, g, b) = colorsys.hsv_to_rgb(i / res, 1.0, 0.9)

        # for j in range(8):
        pxl.write(bytearray([math.floor(r * 0x100), math.floor(g * 0x100), math.floor(b * 0x100)]))
        pxl.flush()
        time.sleep(0.01)
