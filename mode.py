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

def sendMessage(controller, message):
    for c in message:
        a = ord(c)
        vibration = XINPUT_VIBRATION(a<<8, (a^0x4b)<<8)
        XInputSetState(0, ctypes.byref(vibration))
        sleep(0.01)
    vibration = XINPUT_VIBRATION(0, 0)
    XInputSetState(0, ctypes.byref(vibration))
    sleep(0.01)
    
def sendCommand(command):
    data = [REPORT_ID] + list(map(ord, command))
    data += [0 for i in range(REPORT_SIZE+1-len(data))]
    myReport.set_raw_data(data)
    myReport.send()
    
def getString():
    data = myReport.get()[1:]
    try:
        end = data.index(0)
        return "".join(chr(a) for a in data[:end])
    except:
        return ""
    
def query(command):
    sendCommand(command+"?")
    t0 = time()
    lastSent = t0
    while time()-t0 < TIMEOUT:
        out = getString()
        if out.startswith(command+"="):
            print(out)
            return out[len(command)+1:]
        if time()-lastSent >= 0.001:
            sendCommand(command+"?")
            lastSent = time()
    return None

msg = False

myReport = None

while myReport is None:    
    for d in hid.HidDeviceFilter(vendor_id = 0x1EAF).get_devices():
        print(d)
        device = d
        device.open()

        for report in device.find_feature_reports():
            if report.report_id == REPORT_ID and report.report_type == "Feature":
                myReport = report
                break
                    
        if myReport is not None:
            id = query("id")
            if id:
                print(device)
                print(id)
            break
        
        myReport = None
        device.close()
    
    if myReport is None and not msg:
        if hid.HidDeviceFilter(vendor_id = 0x045e, product_id=0x028e).get_devices():
            print("You may be in XBox360 mode. Attempting to force exit.")
            for i in range(5):
                try:
                    sendMessage(0, 'Exit2HID')
                except:
                    pass
        else:
            print("Plug device in (or press ctrl-c to exit).")
        msg = True
    
    sleep(0.25)
    
if len(argv)>1:
    sendCommand("m:"+argv[1])
    print("Mode set to: "+query("M"))
    device.close()
else:
    try:
        from tkinter import * 
    except ImportError:
        from Tkinter import *
    
    root = Tk()
    root.title("Mode")

    option = Listbox(root,selectmode=SINGLE)
    option.config(width=0)
    option.config(height=0)
    option.pack()
    
    current = query("M")

    selection = 0
    i = 0
    while True:
        n = query("M"+str(i))
        if n is None or n == "":
            break
        option.insert(END, n)
        if n == current:
            option.select_set(i)
            option.activate(i)
            selection = i
        i+=1

    def up(_):
        global selection
        if selection > 0:
            option.select_clear(selection)
            selection -= 1
            option.select_set(selection)
            option.activate(selection)
            
    def down(_):
        global selection
        if selection < option.size():
            option.select_clear(selection)
            selection += 1
            option.select_set(selection)
            option.activate(selection)
            
    def ok(*args):
        opt = option.get(ACTIVE)
        sendCommand("M:"+opt)
        if query("M") != opt:
            print("Error setting mode")
        else:
            print("Set to: "+opt)
        device.close()
        root.quit()
        
    def cancel(*args):
        device.close()
        root.quit()

    root.bind("<Down>", down)
    root.bind("<Up>", up)
    root.bind("<Return>", ok)
    root.bind("<Escape>", cancel)
        
    bottom = Frame(root)
    bottom.pack()

    button = Button(root, text="OK", command=ok)
    button.pack(in_=bottom,side=LEFT)
    button2 = Button(root, text="Cancel", command=cancel)
    button2.pack()
    button2.pack(in_=bottom,side=RIGHT)

    mainloop()
        