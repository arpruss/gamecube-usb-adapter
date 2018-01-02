from pywinusb import hid
from time import sleep,time
from sys import argv

TIMEOUT = 1
REPORT_ID = 20
REPORT_SIZE = 63
HID_REPORT_FEATURE = 3


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

msg = False

myReport = None

while myReport is None:
    
    for d in hid.HidDeviceFilter(vendor_id = 0x1EAF).get_devices():
        device = d
        device.open()

        for report in device.find_feature_reports():
            if report.report_id == REPORT_ID and report.report_type == "Feature":
                myReport = report
                break
                    
        if myReport is not None and query("id"):
            break
        
        myReport = None
        device.close()
    
    if myReport is None and not msg:
        print("Plug device in (or press ctrl-c to exit)")
        msg = True
    
    sleep(0.25)
    
print(device)    
                
if len(argv)>1:
    sendCommand("m:"+argv[1])
    print("Mode set to: "+query("M"))
    device.close()
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
        device.close()
        root.quit()

    button = Button(root, text="OK", command=ok)
    button.pack()

    mainloop()
        