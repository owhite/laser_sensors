#!/usr/bin/env python

import time
import serial
from probe_device import *
from Tkinter import *

class layout:
   def __init__(self, master):
      self.master = master
      self.frame = Frame(self.master)
      self.button_num = 12
      Label(self.frame, text="Buttons:").grid(row=0, sticky=W)

      self.vars = []
      for i in range(self.button_num):
         self.vars.append(IntVar())

      x = self.button_num / 2
      for i in range(self.button_num / 2):
         Checkbutton(self.frame, text="B%d" % (i + 1),
                     variable=self.vars[i]).grid(row = i+1, column = 0, sticky=W)
         Checkbutton(self.frame, text="B%d" % (x + i + 1),
                     variable=self.vars[i + x]).grid(row = i+1, column = 1, sticky=W) 

      Button(self.frame, text='Load', command=self.write_serial).grid(row=self.button_num + 1,
                                                                      sticky=W, pady=4)
      Button(self.frame, text='Quit', command=master.quit).grid(row=self.button_num + 2,
                                                                sticky=W, pady=4)
      self.frame.pack()

      l = probe_device('relay_driver', 115200)
      if l.port_found:
         self.serial = l.serial
         self.serial_exists = 1

   def write_serial(self):
      if (self.serial_exists):
         self.serial.flushInput()  # flush input buffer
         self.serial.flushOutput() # flush output buffer
         for i in range(self.button_num):
            cmd = "$ set_relay %d %d\n" % (i + 1, self.vars[i].get())
            self.serial.write(cmd)
            self.serial.flushOutput() # flush output buffer
      else:
         print "i got nothing"


def main(): 
    root = Tk()
    app = layout(root)
    root.mainloop()

if __name__ == '__main__':
    main()


