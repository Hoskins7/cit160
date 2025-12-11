#!/usr/bin/python3
# copyright 2022 by Peter Dordal
# licensed under the Apache 2.0 license
 
# generates exponentially random UDP traffic
# This works better than TCP as small TCP segments sent within a few ms 
# of one another tend to get consolidated into one larger segment.
# sleep...send...sleep...send...
# receiver: netcat -u -l 5433 >/dev/null &

from socket import *
from sys import argv
import os
import time
import threading
import math
import random

default_host = "localhost"
portnum = 5433
density = 0.02  	# fraction of bottleneck link used by this traffic. 0.01 is standard
packetsize = 210	# considerably larger than real telnet
BottleneckBW = 40	# mbps
# BottleneckBW = 0.2	# should yield mean spacing of 1.0 sec
RTMAX = 0.5		# don't wait more than 1/2 sec

letters64 = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJLKLMNOPQRSTUVWXYZ0123456789,.'

def talk():
        global default_host, portnum, density, packetsize
        rhost = default_host
        if len(argv) > 1:
            rhost = argv[1]
        if len(argv) > 2:
            portnum = int(argv[2])
        print("Looking up address of " + rhost + "...", end="")
        try:
            dest = gethostbyname(rhost)
        except gaierror as mesg:
            errno,errstr=mesg.args
            print("\n   ", errstr);
            return;
        print("got it: " + dest)
        addr=(dest, portnum)
        s = socket(AF_INET, SOCK_DGRAM)

        buf = bytearray(os.urandom(packetsize))
        str=''
        for i in range(packetsize-1):
            b = buf[i]
            b = b & ((1<<6) - 1)
            str += (letters64[b])
        str += '\n'
        buf = bytes(str, 'ascii')
        print('length of buf is', len(buf))

        meanspacing = spacing(BottleneckBW, density)
        print('meanspacing={}ms'.format(1000*meanspacing))
        count=0
        maxcount = 0

        while True:
            count += 1
            rt = rtime(meanspacing)
            if rt > RTMAX: rt = RTMAX
            time.sleep(rt)
            try:             s.sendto(buf, addr)
            except: 
                print('sendto failed to {}'.format(addr))
                return
            if maxcount > 0 and count > maxcount: break

def spacing(mbps, density):
    global packetsize
    # megabit/sec = kbit/ms
    # time to send 1:
    sizeKB = (packetsize+40.0)/1000
    timetosend = sizeKB*8.0/mbps	# in ms
    return timetosend/density/1000.0
        
# ms is the mean packet spacing in units of ms
def rtime(ms):
    x = random.random()
    if x==0: x = 0.0001
    if x==1: x = 0.99
    return -math.log(x)*ms
                
talk()
