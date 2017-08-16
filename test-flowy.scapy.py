#!/usr/bin/python

import sys
from scapy.all import *

for i in range (1, 7):
	ip = "192.168.1." + str(i)
	p = Ether(src="02:00:00:01:02:3" + str(i), dst="24:8a:07:8d:ae:ae")/Dot1Q(vlan = 0x123)/IP(dst = ip)
	sendp(p, iface = 'enp130s0f0', count=1)

print("-------------------")

for i in range (1, 3):
	ip = "192.168.1." + str(i)
	p = Ether(src="02:00:00:01:02:3" + str(i), dst="24:8a:07:8d:ae:00")/IP(dst = ip)
	sendp(p, iface = 'enp130s0f0', count=1)

for i in range (1, 3):
	ip = "192.168.1." + str(i)
	p = Ether(src="02:00:00:01:02:31" , dst="24:8a:07:8d:ae:01")/Dot1Q(vlan = 0x123)/IP(dst = ip)
	sendp(p, iface = 'enp130s0f0', count=1)

p = Ether(src="04:00:00:01:02:34", dst="ff:ff:ff:ff:ff:ff")/Dot1Q(vlan = 0x123)/ARP()
p.add_payload('F'*(64 - len(p)))
sendp(p, iface = 'enp130s0f1', count=1)
