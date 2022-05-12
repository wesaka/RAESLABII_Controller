#!/usr/bin/env python
#
# Redirect data from a serial port (bluetooth) connection to a serial port and vice versa.
#
##########################################################################
# Project           : RES_Class, Serial to Serial fowarder
#
# Program name      : serial2bluetooth.py
#
# Author            : Jong-Hoon Kim
#
# Date created      : 05/04/2022
#
# Purpose           : Message fowarding from USB serial (Arduino) to Bluetooth
#
#
# Revision History  :
#
# Date        Author      Ref    Revision (Date in MMDDYYYY format)
# MMDDYYYY    name      1  v-xx   revision note.
#
##########################################################################
##########################################################################
#   Instructional Note:
#
#
##########################################################################


from time import sleep
import serial
import threading
import time
import sys

######################################################################
# Bluetooth port setup. depend your pc, port number will be differet
######################################################################
sUSB = serial.Serial('/dev/ttyACM0', 38400)  # Establish the connection on a specific port
sBluetooth = serial.Serial('/dev/rfcomm0', 38400)  # Establish the connection on a specific port

myLock = threading.Lock()


def thread_bluetooth(event):
    while not event.is_set():
        try:
            msg = sBluetooth.readline()  # Read the newest output from the Arduino
            #      print ((msg).decode())
            #      tmpStr = msg.split( )
            while True:
                ######################################################################
                #        Message Forwarding from bluetooth to usbserial(Arduino)
                ######################################################################
                if myLock.acquire():  # lock for sharing ser object
                    myLock.locked()
                    sUSB.write(msg)
                    sUSB.flush()
                    myLock.release()
                    break
                else:
                    print ("block1")

        except ValueError:
            pass
        time.sleep(0.01)


def thread_usbserial(event):
    while not event.is_set():
        try:
            msg = sUSB.readline()  # Read the newest output from the Arduino
            print (msg.decode())
            # tmpStr = msg.split( )
            while True:
                ######################################################################
                #        Message Forwarding from bluetooth to usbserial(Arduino)
                ######################################################################
                if myLock.acquire():  # lock for sharing ser object
                    myLock.locked()
                    sBluetooth.write(msg)
                    sBluetooth.flush()
                    myLock.release()
                    break
                else:
                    print ("block1")

        #   if str[0] == b"joy" :
        #     print ("joymessage")
        # ######################################################################
        # #       Parsing a Joystic Message for extra control steps
        # #		You should implement your messase pasring code (use a simple protocal)
        # #		(e.g) first string "joy" as the target message
        # ######################################################################

        except ValueError:
            pass
        time.sleep(0.01)


event = threading.Event()
y = threading.Thread(target=thread_usbserial, args=(event,))
y.start()
time.sleep(3)
x = threading.Thread(target=thread_bluetooth, args=(event,))
x.start()

while True:
    try:
        time.sleep(0.1)
    except KeyboardInterrupt:
        event.set()  # inform the child thread that it should exit
        x.join()
        y.join()
        break

sys.stderr.write('\n--- exit ---\n')



