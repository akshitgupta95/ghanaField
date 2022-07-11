# Lora Node
from network import LoRa
import socket
import binascii
import struct
import time
import pycom

# SPS30 Sensor
from time import time, sleep
import sys
from SPS30 import SPS30

# import config

LORA_FREQUENCY = 433175000
LORA_NODE_DR = 5

lora = LoRa(mode=LoRa.LORAWAN, region=LoRa.EU433)  # Australia

# create an ABP authentication params
dev_addr = struct.unpack(">l", binascii.unhexlify("260B0701"))[0]
nwk_swkey = binascii.unhexlify("BF6F735B7728AD9AEA7DAE6B423E7E15")
app_swkey = binascii.unhexlify("EF1D59AC28332E8C836302F94C89407C")

# remove all the non-default channels
for i in range(3, 16):
    lora.remove_channel(i)

# set the 3 default channels to the same frequency
lora.add_channel(0, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)
lora.add_channel(1, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)
lora.add_channel(2, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)

# join a network using ABP (Activation By Personalization)
lora.join(activation=LoRa.ABP, auth=(dev_addr, nwk_swkey, app_swkey))

# create a LoRa socket
s = socket.socket(socket.AF_LORA, socket.SOCK_RAW)

# set the LoRaWAN data rate
s.setsockopt(socket.SOL_LORA, socket.SO_DR, 5)

# make the socket blocking
s.setblocking(False)
pycom.heartbeat(False)
pycom.rgbled(0xFF00)

print("Started LoRa Node at :", LORA_FREQUENCY, "Hz")


interval = 30
sample = 60
debug = True

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
#print(sps30.device_info(debug=debug))
# print(sps30.isStarted(debug=debug))

class SensorData:
    
    def __init__(self,pm1,pm25,pm4,pm10,pm05_cnt,pm1_ccnt,pm25_cnt,pm4_cnt,pm10_cnt):
        self.pm1 = pm1
        self.pm25 = pm25
        self.pm4 = pm4
        self.pm10 = pm10
        self.pm05_cnt = pm05_cnt
        self.pm1_ccnt = pm1_ccnt
        self.pm25_cnt = pm25_cnt
        self.pm4_cnt = pm4_cnt
        self.pm10_cnt = pm10_cnt
    
    def export_data(self):
        return [self.pm25, self.pm10,self.pm25_cnt,self.pm10_cnt]

while True:
    lastTime = time()
    # print(sps30.getData(debug=debug))
    s.send(sps30.read_measurement(debug=debug))

    now = interval + time() - lastTime
    rx, port = s.recvfrom(256)
    if rx:
        print("Received: {}, on port: {}".format(rx, port))
    if now > 0:
        sleep(now)



# for i in range(200):

#     pkt = b"PKT #" + bytes([i])
#     print("Sending:", pkt)
#     s.send(pkt)
#     time.sleep(4)
#     rx, port = s.recvfrom(256)
#     if rx:
#         print("Received: {}, on port: {}".format(rx, port))
#     time.sleep(5)
