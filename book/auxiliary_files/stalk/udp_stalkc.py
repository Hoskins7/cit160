#!/usr/bin/python3

from socket import *
from sys import argv

portnum = 5432

def talk():
        rhost = "localhost"
        if len(argv) > 1:
            rhost = argv[1]
        print("Looking up address of " + rhost + "...", end="")
        try:
            dest = gethostbyname(rhost)
        except (GAIerror, herror) as mesg:     # GAIerror: error in GetAddrInfo()
            errno,errstr=mesg.args
            print("\n   ", errstr);
            return;
        print("got it: " + dest)
        addr=(dest, portnum)                  # a socket address
        s = socket(AF_INET, SOCK_DGRAM)
        s.settimeout(1.5)              # we don't actually need to set timeout here
        while True:
            try:
                buf = input("> ")
            except:
                break
            s.sendto(bytes(buf + "\n", 'ascii'), addr)
                
talk()
