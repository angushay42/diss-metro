import serial
from serial import SerialException
from collections import defaultdict
import json
import time
import pathlib  
import os
import sys


# port_name = "/dev/tty.usbmodem11203"

ports = [x for x in os.listdir("/dev") if x.startswith("tty.") and "usbmodem" in x]
assert ports
port_name = f"/dev/{ports[0]}"
file_name = pathlib.Path(__file__).parent / "test.json"



def get_fake_data():
    # lets do 1097 = 0b0000 0100 0100 1001
    # so 0x0449
    # so we send 0x4 and 0x49
    # it is actually the other way around
    fake = bytes([0x49, 0x04])
    return fake

def get_int(ser: serial.Serial, len: int, signed: bool = True):
    return int.from_bytes(ser.read(len), byteorder='little', signed=signed)

def recv_double_words():
    # open port
    try:
        ser = serial.Serial(port_name, 115200, 8, "N", 1)
    except SerialException as e:
        raise BaseException(*e)     # todo

    data = []
    s = ""
    try: 
        while True:
            # program will send over in lsb form
            if s and s[-1] == "e":
                s = ""
                double = get_int(ser, 4)
                data.append(double)
                print(f" = {double}")
                continue
            try:
                byte = get_int(ser, 2)
                if chr(byte) in "double":
                    s+= chr(byte)
                    print(chr(byte), end="")
            except ValueError:
                print(f"ERROR: {byte}")
            
    except KeyboardInterrupt:
        pass
    print() # annoying newline
    return data

def recv_words():
    # open port
    try:
        ser = serial.Serial(port_name, 115200, 8, "N", 1)
    except SerialException as e:
        raise BaseException(*e)     # todo

    data = []
    try: 
        while True:
            # program will send over in lsb form
            word = int.from_bytes(ser.read(2), byteorder='little', signed=True)
            if word < -(2**13) or word > (2**13)-1:
                ser.read(1)     # skip a byte if they are in wrong order
            data.append(word)
            print(f"new word: {data[-1]}")
    except KeyboardInterrupt:
        pass
    print() # annoying newline
    return data

def recv_bytes():
    # open port
    try:
        ser = serial.Serial(port_name, 115200, 8, "N", 1)
    except SerialException as e:
        raise BaseException(*e)     # todo

    data = defaultdict(list)
    s = ""
    try: 
        while True:
            # program will send over in lsb form
            if s and s[-1] == ":":
                byte = int.from_bytes(ser.read(4), byteorder="little", signed=False)
                print(f"{s} {byte}")
                continue
            byte = int.from_bytes(ser.read(2), byteorder='little', signed=True)
            if byte >= 1 << 8:
                print(f"tempo: {byte}")
            try:
                s+= chr(byte)
                print(f"{chr(byte)}", end="")
                now = time.time()
                if chr(byte) == '\n':
                    print(f" at {now}")
                    data[s].append(now)
                    s = ""
            except ValueError:
                print(f"ERROR: {byte}", end="\n\n")
            # print(f"new word: {data[-1]}/{chr(data[-1])}")
    except KeyboardInterrupt:
        pass
    print() # annoying newline
    return data

def recv_64():
    # open port
    try:
        ser = serial.Serial(port_name, 115200, 8, "N", 1)
    except SerialException as e:
        raise BaseException(*e)     # todo

    data = []
    try: 
        while True:
            # program will send over in lsb form
            try:
                byte = get_int(ser, 8, False)
                print(byte, end="\n")
            except ValueError:
                print(f"ERROR: {byte}")
            
    except KeyboardInterrupt:
        pass
    print() # annoying newline
    return data

def recv_basic():
    # open port
    try:
        ser = serial.Serial(port_name, 115200, 8, "N", 1)
    except SerialException as e:
        raise BaseException(*e)     # todo

    data = []
    try: 
        while True:
            # program will send over in lsb form
            try:
                byte = get_int(ser, 2)
                if byte > (2**8):
                    byte = (byte << 16) | get_int(ser, 2)
                    print(f" {byte}")
                else: 
                    print(chr(byte), end="")
            except ValueError:
                print(f"ERROR: {byte}")
            
    except KeyboardInterrupt:
        pass
    print() # annoying newline
    return data


def test_fake_data(data:bytes) -> int:
    return int.from_bytes(data, byteorder='little', signed=True)

def test_main():
    print(test_fake_data(get_fake_data()))

def analyse(data: dict[str, list]):
    keys = {
        "before_check": "before isr check\n",
        "after_check": "after isr check\n",
        "after_clear": "after isr clear\n"
    }

    if not all([x in data for x in keys.values()]):
        raise TypeError     # base exception

    # get the total time from before to end
    time_spent = [abs(x[0] - x[1]) for x in zip(data[keys["before_check"]], data[keys["after_clear"]])]
    i = 0
    total = 0
    for t in time_spent:
        if total >= 1:
            break
        total += t
        i+= 1
    print(i)
    print( sum(time_spent) / len(time_spent))



def main():
    args = sys.argv[1:]
    # if args and args[0] == "w":
    #     func = recv_words
    # else:
    #     func = recv_bytes

    # func = recv_double_words
    func = recv_basic

    start = time.time()
    data = func()
    end = time.time()

    analyse(data)



    # obj = {
    #     "time": {
    #         "start":    start,
    #         "end":      end,
    #         "diff":     end - start
    #     },
    #     "data": data
    # }
    # with open(file_name, "w") as f:
    #     json.dump(obj, f, indent=2)
    print("done!")



if __name__ == "__main__":
    print(port_name)
    main()

    
