#!/usr/bin/python

# Copyright 2017-2022 by Peter Dordal
# licensed under the Apache 2.0 license

# Uses packet capture to monitor the cwnd of two connections.
# writes cwnd periodically to stdout
# this version monitors TWO different interfaces.
# Output is written as (time,intf1, intf2)

# This version uses the libpcap library, which is a so-called "ctypes" library.
# This means that some of the code looks more like C than Python.
# The ctypes documentation is recommended!
# Interface names and IP addresses are specific to those created by competition.py

from ctypes import *
import libpcap			# see https://libpcap.readthedocs.io/en/latest/
				# Based on ctypes, which is a very lightweight Python wrapper for C libraries
				# Not very Pythonic!
# from packet import *
import time
import threading
import sys
import dpkt

port1 = 5430
port2 = 5431
seq1  = -1
ack1  = -1
seq2  = -1
ack2  = -1
starttime = -1
pktcount = 0		# count of packets
pktprev  = 0
repeats = 0
halting = False
statcount = 0
interval = 0.1
dev1 = 'r-eth1'
dev2 = 'r-eth2'
FILTER1 = 'host 10.0.3.10 and tcp and port 5430'	
FILTER2 = 'host 10.0.3.10 and tcp and port 5431'	

demofilter='host 10.2.5.36 and tcp and portrange 5430-5431'

# both capture objects use the same filter, but different devs
# dev is a Python string
# most, but not all, of the messiness resulting from libpcap's being based on ctypes is hidden within make_cap()
def make_cap(dev, filter):
    print('starting make_cap')
    errbuf = c_char_p(bytes(100))
    cdev = c_char_p(bytes(dev, 'ascii'))
    cap = libpcap.create(cdev, errbuf)
    if not cap:
        error('pcap create failed: {}'.format(errbuf.value))
        return None
    libpcap.set_timeout(cap, 100)	# in ms
    libpcap.set_buffer_size(cap, 256)	# 256 bytes
    res = libpcap.activate(cap)
    if res != 0:
        error('pcap activate failed: {}. Are you root?'.format(res))
        return None
    bfilter = bytes(filter, 'ascii')
    cfilter = c_char_p(bfilter)
    bpf_arg = libpcap.bpf_program()		# returns an object that we use to pass the compiled bpf 
    res = libpcap.compile(cap, bpf_arg, cfilter, 0, libpcap.PCAP_NETMASK_UNKNOWN)  # (type, max, filter, optimize?, netmask)
    if res != 0:
        error('pcap compile failed; filter {}'.format(cfilter.value))
        return None
    res = libpcap.setfilter(cap, bpf_arg)
    if res != 0:
        error('pcap setfilter failed; filter {}'.format(cfilter.value))
        return None
    print('done with make_cap')
    return cap
    
def error(str):
    print(str)

# The process_packets functions use global variables, as they are used as thread entry points.
# They compare the absolute seq number of outbound packets with the absolute ack number of returning ACKs.
# Their *difference* is the same as the difference of the two relative numbers. We never determine the Initial Seq Numbers.
def process_packets1():
    global cap1, seq1, ack1, starttime, pktcount
    pkthdr  = libpcap.pkthdr()		# returns an object that is used to return (timestamp, len) info
    print('starting process_packets1')
    while True:
        # print('process_packets1: calling libpcap.next()')
        p = libpcap.next(cap1, pkthdr)
        # print('process_packets1 got a packet')
        pc = cast(p, POINTER(c_ubyte * pkthdr.caplen))		# ctypes-ism
        if not pc: continue
        pbuf = bytes(pc.contents)					# another ctypes-ism
        if halting: exit(0)
        if pbuf == None or len(pbuf) == 0:
             #print '.',
             continue;
        pktcount += 1
        if starttime == -1: 
             starttime = time.time()
             # print 
             printstats()
        ppres = parsepacket(pbuf)
        if not ppres: continue
        (sport, dport, seq, ack, data) = ppres
        if dport == port1:             # get seq if dport in [port1,port2]
            seq1 = seq + len(data)
        elif sport == port1:
            ack1 = ack

def process_packets2():
    global cap2, seq2, ack2, starttime, pktcount
    pkthdr  = libpcap.pkthdr()		# returns an object that is used to return (timestamp, len) info
    print('starting process_packets2')
    while True:
        # print('process_packets2: calling libpcap.next()')
        p = libpcap.next(cap2, pkthdr)
        # print('process_packets2 got a packet')
        pc = cast(p, POINTER(c_ubyte * pkthdr.caplen))
        if not pc: continue
        pbuf = bytes(pc.contents)
        if halting: exit(0)
        if pbuf == None or len(pbuf) == 0:
             #print '.',
             continue;
        pktcount += 1
        if starttime == -1: 
             starttime = time.time()
             printstats()
        ppres = parsepacket(pbuf)
        if not ppres: continue
        (sport, dport, seq, ack, data) = ppres
        if dport == port2:             # get seq if dport in [port1,port2]
            seq2 = seq + len(data)
        elif sport == port2:
            ack2 = ack


# return (sport, dport, seq, ack, data)
def parsepacket(p):
    eth = dpkt.ethernet.Ethernet(p)
    if not isinstance(eth.data, dpkt.ip.IP): return None
    ip = eth.data
    if not isinstance(ip.data, dpkt.tcp.TCP): return None
    tcp = ip.data
    return (tcp.sport, tcp.dport, tcp.seq, tcp.ack, (tcp.data))


# started by whichever thread sees first packet
# Not AT ALL thread-safe, but the odds of even a single miscalculation are quite low
def printstats():
    global starttime, statcount, count, pktprev, repeats, halting  # repeats is global
    elapsed = time.time()-starttime
    if starttime != -1:
        # we really should check that seq1, ack1, seq2 and ack2 have all received a value at least once
        print ('{}\t{}\t{}'.format(elapsed, sub32(seq1,ack1), sub32(seq2,ack2)))		# should be 32-bit differences
        print ('{}\t{}\t{}'.format(elapsed, sub32(seq1,ack1), sub32(seq2,ack2)))
    if pktcount > 0 and pktcount == pktprev:	# quit when there's no new packets
        if repeats >= 10:
             halting=True
             print('exiting after 10 repeats with no change')
             exit(0)
        repeats+=1
    elif pktcount > 0:
       pktprev = pktcount
       repeats=0
    statcount +=1
    nexttime = starttime + statcount * interval
    inter = nexttime - time.time()
    inter = statcount * interval - elapsed
    if starttime > -1: assert inter > 0, 'printstats: bad interval {}'.format(inter)
    t = threading.Timer(inter,printstats)
    t.start()

two32 = 1<<32		# 2*32

def sub32(a,b):
    return (a-b) % two32

def tmain():
    global cap1, cap2
    libpcap.config(LIBPCAP=None)
    cap1 = make_cap(dev1, FILTER1)
    if not cap1: 
        print('make_cap cap1 failed')
        return
    cap2 = make_cap(dev2, FILTER2)
    if not cap2:
        print('make_cap cap2 failed')
        return
    th1 = threading.Thread(target=process_packets1, name="interface1")
    th2 = threading.Thread(target=process_packets2, name="interface2")
    # th1.daemon = True
    # th2.daemon = True
    print('starting threads')
    th1.start()
    th2.start()
    # printstats()			# don't need as this is called by one of the other threads


dev = 'tap0'

def demo():
    libpcap.config(LIBPCAP=None)
    cap = make_cap(dev, demofilter)
    pkthdr  = libpcap.pkthdr()		# returns an object that is used to return (timestamp, len) info
    # pktdata = libpcap.pktdata()
    # pktdata = POINTER(c_ubytes(bytes(300)))
    # pktdata = create_string_buffer(bytes(300))
    # print('make_cap returned, type={}'.format(type(cap)))
    for i in range(10):
        p = libpcap.next(cap, pkthdr)
        pc = cast(p, POINTER(c_ubyte * pkthdr.caplen))
        pbuf = bytes(pc.contents)
        ppres = parsepacket(pbuf)
        # print('length of packet is {}'.format(len(pbuf)))
        # iph = ip4header.read(pbuf, ETHHDRLEN)
        # tcph= tcpheader.read(pbuf, ETHHDRLEN + iph.iphdrlen)
        # print(tcph.absseqnum, tcph.absacknum)
        # data = pbuf[ETHHDRLEN + iph.iphdrlen + tcph.tcphdrlen :]
        # print('data={}'.format(data))
        print(ppres)
        
        
tmain()
