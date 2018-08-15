from sys import argv,exit
import ctypes

try:
    from pywinusb import hid
except ImportError:
    exit("You need pywinusb. Run python -m pip install pywinusb")
    
from time import sleep,time

TIMEOUT = 1
REPORT_ID = 20
REPORT_SIZE = 63
HID_REPORT_FEATURE = 3

class XINPUT_VIBRATION(ctypes.Structure):
    _fields_ = [("wLeftMotorSpeed", ctypes.c_ushort),
                ("wRightMotorSpeed", ctypes.c_ushort)]

xinput = ctypes.windll.xinput1_1  # Load Xinput.dll

XInputSetState = xinput.XInputSetState
XInputSetState.argtypes = [ctypes.c_uint, ctypes.POINTER(XINPUT_VIBRATION)]
XInputSetState.restype = ctypes.c_uint

vibration = XINPUT_VIBRATION(0xFF<<8, 0xFF<<8)
XInputSetState(0, ctypes.byref(vibration))
t = 60 if len(argv) < 2 else float(argv[1])
sleep(t)
vibration = XINPUT_VIBRATION(0, 0)
XInputSetState(0, ctypes.byref(vibration))

