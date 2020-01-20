#!/usr/bin/env python


import sys
import serial
import json
import pickledb


(me, device, command) = sys.argv

db = pickledb.load('example.db', False)

with serial.Serial('/dev/cu.SLAB_USBtoUART', 115200) as ser:
    key = "%s/%s" % (device, command)
    cmd = db.get(key)
    cmd['act'] = 'tra'
    ser.write(bytes(json.dumps(cmd, separators=(',', ':')),'US-ASCII'))
    ser.write(b'\n')
    line = ser.readline()   # read a '\n' terminated line
    resp = json.loads(line)
    if resp["ok"]:
        print("OK")
    else:
        print(resp["err"])
        exit(1)

