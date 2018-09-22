#!/usr/bin/python

import time
import serial
import sys

JOY_SLAVE_ADDRESS          = 0x0A
FLOW_SLAVE_ADDRESS         = 0x0B
LOADCELL_SLAVE_ADDRESS     = 0x0C
RELAYDRIVER_SLAVE_ADDRESS  = 0x0D

I2C_DONOTHING  = 0x00
I2C_REPORT     = 0x0A
I2C_ACTIVATE   = 0x0B
I2C_DEACTIVATE = 0x0C
I2C_ALARM      = 0x0D
I2C_SETPINS    = 0x10
I2C_SHUTDOWN   = 0x11

HELLO             = 0x01
ALREADY_CONNECTED = 0x02
SHUTDOWN          = 0x03
ERROR             = 0x04
WRITE_DEVICE      = 0x05
READ_DEVICE       = 0x06
SET_DEVICE        = 0x07

PACKET_SIZE = 6

def read_all(ser, chunk_size = 200):
    if not ser.timeout:
        raise TypeError('requires that timeout is set')
    read_buffer = b''

    while True:
        byte_chunk = bytearray(ser.read(size=chunk_size))
        read_buffer += byte_chunk
        if not len(byte_chunk) == chunk_size:
            break

    return read_buffer

def still_connected(ser): 
    ser.write(bytearray(['<',HELLO,'>']))
    msg = read_all(ser)
    if (msg[1] == ALREADY_CONNECTED):
        return(True)
    return(False)

try:
    port = '/dev/cu.usbmodem3896411'
    s = serial.Serial(port, 115200, timeout=0.01)
    s.write(bytearray(['<',HELLO,'>']))
    msg = read_all(s)
    if (msg[1] not in [ALREADY_CONNECTED, HELLO]):
        sys.exit("no connection")

except (OSError, serial.SerialException):
    print "Probe received SerialException"
    sys.exit()

count = 0
while still_connected(s):
    s.write(bytearray(['<',WRITE_DEVICE,LOADCELL_SLAVE_ADDRESS,I2C_SETPINS,0,1,2,'>']))
    msg = read_all(s, PACKET_SIZE)
    s_ = str(count) + " "
    for b in msg:
        s_ = s_ + str(b) + " "

    print s_
    count = count + 1
    # time.sleep(.06)
