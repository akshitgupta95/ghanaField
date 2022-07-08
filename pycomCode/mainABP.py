from network import LoRa
import socket
import binascii
import struct
import time
import pycom


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

for i in range(200):

    pkt = b"PKT #" + bytes([i])
    print("Sending:", pkt)
    s.send(pkt)
    time.sleep(4)
    rx, port = s.recvfrom(256)
    if rx:
        print("Received: {}, on port: {}".format(rx, port))
    time.sleep(5)
