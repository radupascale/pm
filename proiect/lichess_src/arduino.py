import serial
import serial.tools.list_ports
import warnings

def list_devices():
    print(serial.tools.list_ports.comports())
    for p in list(serial.tools.list_ports.comports()):
        print(p)