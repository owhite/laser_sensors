#!/usr/bin/env python

import time
import serial
from probe_device import *
from Tkinter import *

class layout:
   def __init__(self, master):
      self.master = master
      self.frame = Frame(self.master)
      self.button_num = 22
      Label(self.frame, text="Buttons:").grid(row=0, sticky=W)

      self.vars = []
      for i in range(self.button_num):
         self.vars.append(IntVar())

      for i in range(self.button_num / 2):
         x = i + (self.button_num / 2)
         Checkbutton(self.frame, text="P%d" % (i),
                     variable=self.vars[i], command=lambda i=i: self.toggle_button(i)).grid(row = i+1, column = 0, sticky=W)
         Checkbutton(self.frame, text="P%d" % (x), variable=self.vars[x], command=lambda i=x: self.toggle_button(i)).grid(row = i+1, column = 1, sticky=W)

      Button(self.frame, text='Quit', command=master.quit).grid(row=self.button_num + 3,
                                                                sticky=W, pady=4)
      self.frame.pack()

      l = probe_device('probe_example', 9600)
      self.serial_exists = 0
      if l.port_found:
         self.serial = l.serial
         self.serial_exists = 1

   def toggle_button(self, num):
      cmd = "$ set_port %d %d\n" % (num + 1, self.vars[num].get())
      if (self.serial_exists):
         self.serial.flushInput()  # flush input buffer
         self.serial.flushOutput() # flush output buffer
         self.serial.write(cmd)
         self.serial.flushOutput() # flush output buffer
      else:
         print "no serial"

   def write_serial(self):
      if (self.serial_exists):
         self.serial.flushInput()  # flush input buffer
         self.serial.flushOutput() # flush output buffer
         for i in range(self.button_num):
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


