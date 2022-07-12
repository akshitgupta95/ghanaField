import sys
from SPS30 import SPS30
from time import time, sleep
import json

interval = 10
sample = 10
debug = False
try:
    from machine import UART, Pin

    pins = ("P4", "P3", "P19")
    print("Using pins: %s" % str(pins))
    Pin(pins[2], mode=Pin.OUT).value(1)
    sleep(1)
    port = UART(1, baudrate=115200, pins=pins[:2], timeout_chars=20)
except:
    import sys

    port = sys.argv[1]

sps30 = SPS30(port=port, debug=debug, sample=sample, interval=interval)

for i in range(4):
    lastTime = time()
    print(json.dumps(sps30.getData(debug=debug)))
    now = interval + time() - lastTime
    if now > 0:
        sleep(now)
