from pywinusb import hid
from time import sleep,time
from sys import argv

TIMEOUT = 1
REPORT_ID = 20
REPORT_SIZE = 63
HID_REPORT_FEATURE = 3

device = hid.HidDeviceFilter(vendor_id = 0x1EAF, product_id = 0x0004).get_devices()[0]
print(device)
device.open()

#def dataHandler(data):
#    if data[0] == HID_REPORT_FEATURE:
#        print(data)

myReport = None
for report in device.find_feature_reports():
    if report.report_id == REPORT_ID and report.report_type == "Feature":
        myReport = report
        break
        
assert myReport is not None

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
            return out[len(command)+1:]
        if time()-lastSent >= 0.001:
            sendCommand(command+"?")
            lastSent = time()
    return None

if len(argv)>1:
    sendCommand("m:"+argv[1])
    print("Mode set to: "+query("M"))
else:
    from tkinter import *
    
    root = Tk()

    option = Listbox(root,selectmode=SINGLE)
    option.config(width=0)
    option.pack()
    
    current = query("M")

    i = 0
    while True:
        n = query("M"+str(i))
        if n is None or n == "":
            break
        option.insert(END, n)
        if n == current:
            option.select_set(i)
            option.activate(i)
        i+=1

    #
    # test stuff

    def ok():
        sendCommand("M:"+option.get(ACTIVE))
        root.quit()

    button = Button(root, text="OK", command=ok)
    button.pack()

    mainloop()