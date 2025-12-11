#!/usr/bin/python3

# copyright 2022 by Peter Dordal
# licensed under the Apache 2.0 license
 
# Given a TCP connection, usually by destination address and dest port,
# it calls ss ('socket statistics') at regular intervals and gets the cwnd.
# It prints time,cwnd at intervals.
# If it gets back two records, it prints "multiple connections" as an error msg
# Example: ss --info --tcp dst lubuntu and dport 5431
#          ss --info --tcp --no-header dst lubuntu:5431
# Note that the cwnd value returned by ss is in packets; we extract mss as well to convert to bytes.

import sys
import subprocess
import time
import threading
import socket

INTERVAL = 0.2
# INTERVAL = 1.0

# can return None, a string, or a numeric value of cwnd
def sscall(desthost, destport):
    sscmd = 'ss -i -t -H dst {}:{}'.format(desthost, destport)
    # print('cmd: "{}"'.format(sscmd))
    byte_res = subprocess.run(sscmd.split(), capture_output=True)
    result = byte_res.stdout.decode('ascii')
    # print('got result: {}'.format(result))
    if result == '': return None		# no connection found
    start = result.find('\n')			# skip first line
    result = result[start+1:]
    second= result.find('\n')
    if second + 1 < len(result):		# more than one connection found
    	return 'multiple connections'				# fixme?
    mss  = getval('mss',  result)
    cwnd = getval('cwnd', result)
    # cwndpos = result.find('cwnd:', start+1)
    # cwndend = result.find(' ', cwndpos)
    # cwnd = int(result[cwndpos+5 : cwndend])
    return mss*cwnd

# finds a value in the ss output string
def getval(valname, ssout):
    valpos = ssout.find(valname+':')
    valend = ssout.find(' ', valpos)
    valstr = ssout[valpos + len(valname) + 1 : valend]
    if not valstr.strip().isnumeric(): return 0
    return int(valstr)
    
# print(sscall('lubuntu', 5431))

NONEMAX = 5	# Give the target connection time to start up

# argv should be desthost destport

def main():
    argv = sys.argv
    assert len(argv) >= 3, 'usage: command desthost destport'
    desthostname = argv[1]
    destport = int(argv[2])
    try:
        dest = socket.gethostbyname(desthostname)
    except socket.gaierror as mesg:	# host not found
        errno,errstr=mesg.args
        print("\n   ", errstr);
        return;
    except socket.herror as mesg:
        errno,errstr=mesg.args
        print("\n HERROR: ", errstr);
        return;
    
    print('ss_cwnd.py starting up; desthost={}. destport={}'.format(dest, destport))
    nonecount = 0
    while True:
        cwnd = sscall(dest, destport)
        if cwnd == None: 
            nonecount+= 1
            print('.', end='')
            if nonecount > NONEMAX: return
        elif isinstance(cwnd, str):
            print('multiple connections found')
        else:
            print('{:.3f}\t{}'.format(time.time(), cwnd))
            nonecount = 0
        time.sleep(INTERVAL)
    
main()

