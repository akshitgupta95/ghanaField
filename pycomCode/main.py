# Lora Node
from network import LoRa
import socket
import binascii
import struct
import time
import pycom
import ubinascii

# SPS30 Sensor
import sys
from SPS30 import SPS30
import json

# import config

LORA_FREQUENCY = 433175000
LORA_NODE_DR = 5

lora = LoRa(
    mode=LoRa.LORAWAN, region=LoRa.EU433, coding_rate=LoRa.CODING_4_5
)  # Australia
# create an OTAA authentication parameters, change them to the provided credentials
# app_eui = ubinascii.unhexlify("0000000000000000")
# app_key = ubinascii.unhexlify("DD919E697C7A2FF75076A1C63D556D46")
# # uncomment to use LoRaWAN application provided dev_eui
# dev_eui = ubinascii.unhexlify("70B3D5499E197F30")
# create an ABP authentication params
dev_addr = struct.unpack(">l", binascii.unhexlify("260B0701"))[0]
nwk_swkey = binascii.unhexlify("4EF48679D2B5FE462ADCEE26C0E05831")
app_swkey = binascii.unhexlify("EF1D59AC28332E8C836302F94C89407C")

# remove all the non-default channels
for i in range(3, 16):
    lora.remove_channel(i)

# set the 3 default channels to the same frequency
lora.add_channel(0, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)
lora.add_channel(1, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)
lora.add_channel(2, frequency=LORA_FREQUENCY, dr_min=5, dr_max=5)

# lora.join(activation=LoRa.OTAA, auth=(dev_eui, app_eui, app_key), timeout=0)
# while not lora.has_joined():
#     time.sleep(2.5)
#     print("Not yet joined...")

# print("Joined")
# join a network using ABP (Activation By Personalization)
lora.join(activation=LoRa.ABP, auth=(dev_addr, nwk_swkey, app_swkey))

# create a LoRa socket
s = socket.socket(socket.AF_LORA, socket.SOCK_RAW)

# set the LoRaWAN data rate
s.setsockopt(socket.SOL_LORA, socket.SO_DR, 5)

# make the socket blocking
# s.setblocking(True)
pycom.heartbeat(False)
pycom.rgbled(0xFF00)
# s.send(bytes([0x01, 0x02, 0x03]))
s.setblocking(False)
# data = s.recv(64)
# print(data)

print("Started LoRa Node at :", LORA_FREQUENCY, "Hz")
interval = 10
sample = 10
debug = False
try:
    from machine import UART, Pin

    pins = ("P4", "P3", "P19")
    print("Using pins: %s" % str(pins))
    Pin(pins[2], mode=Pin.OUT).value(1)
    time.sleep(1)
    port = UART(1, baudrate=115200, pins=pins[:2], timeout_chars=20)
except:
    import sys

    port = sys.argv[1]

sps30 = SPS30(port=port, debug=debug, sample=sample, interval=interval)

while True:
    lastTime = time.time()
    data = json.dumps(sps30.getData(debug=debug))
    print(data)
    s.send(data)
    now = interval + time.time() - lastTime
    if now > 0:
        time.sleep(now)

