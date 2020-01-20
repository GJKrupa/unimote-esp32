#!/usr/bin/env python


import sys
import serial
import json
import pickledb


(me, device, command) = sys.argv

db = pickledb.load('example.db', False)

with serial.Serial('/dev/cu.SLAB_USBtoUART', 115200) as ser:
    cmd = { "act": "rec" }
    ser.write(bytes(json.dumps(cmd, separators=(',', ':')),'US-ASCII'))
    ser.write(b'\n')
    line = ser.readline()   # read a '\n' terminated line
    resp = json.loads(line)
    if resp["ok"]:
        print("Recording...")
        try:
            line = ser.readline()
            print(line)
            resp = json.loads(line)
            key = "%s/%s" % (device, command)
            db.set(key, resp)
            db.dump()
            print("%s %s recorded" % (device, command))
        except KeyboardInterrupt:
            cmd = { "act": "can" }
            ser.write(bytes(json.dumps(cmd, separators=(',', ':')),'US-ASCII'))
            ser.write(b'\n')
            line = ser.readline()   # read a '\n' terminated line
            print(line)
    else:
        print(resp["err"])
        exit(1)

