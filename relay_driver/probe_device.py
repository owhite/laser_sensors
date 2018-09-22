#!/usr/bin/env python

import time
import sys
import glob
import serial

class probe_device:
    def __init__(self, device, baud_rate):
        if sys.platform.startswith('win'):
            ttys = ['COM%s' % (i + 1) for i in range(256)]
        elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
           # this excludes if current terminal "/dev/tty"
            ttys = glob.glob('/dev/tty[A-Za-z]*')
        elif sys.platform.startswith('darwin'):
            ttys = glob.glob('/dev/tty.*')
        else:
            raise EnvironmentError('Unsupported platform')

        self.port_found = 0

        for port in ttys:
            try:
                s = serial.Serial(port, baud_rate)
                s.flushInput()  # flush input buffer
                s.flushOutput() # flush output buffer
                cmd = '$ probe_device\n'
                s.write(cmd)
                time.sleep(.3) 
                msg = s.read(s.inWaiting())
                if (msg == device):
                    self.port_found = 1
                    self.serial = s
                    self.port_name = port
                else:
                    s.close()

            except (OSError, serial.SerialException):
                print "Probe received SerialException"

if __name__ == '__main__':
    l = probe_device('relay_driver', 115200)
    if l.port_found:
        print l.port_name
    while ( 1 ):
        pass
