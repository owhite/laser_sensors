#!/usr/bin/env python2.7

from socket import *
import time
from time import gmtime, strftime
import os

address= ('10.0.1.13', 5000) # define server IP and port
client_socket =socket(AF_INET, SOCK_DGRAM) # Set up the Socket
client_socket.settimeout(1) # Only wait 1 second for a response

def ping_device(address, cmd):
    client_socket.sendto(cmd, address) # Send the data request
    d = {}
    d['TIME'] = strftime("%Y-%m-%d %H:%M:%S", gmtime())
    d['SOCKET'] = address

    try:
        rec_data, addr = client_socket.recvfrom(2048) 
        d['STATUS'] = 'UP'
        for i in rec_data.split('\t'):
            key, value = i.split(':', 1)
            d[key] = value

    except:
        d['STATUS'] = 'DOWN'
        pass

    return d


while(1):
    d = ping_device(address, "GO")
    print d

    time.sleep(.5) 

