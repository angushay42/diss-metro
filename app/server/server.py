import serial
from serial import SerialException
from collections import defaultdict
import math
import time
import pathlib  
import os
import sys


class ServerException(BaseException):
    pass

class UART:
    port_name = ""
    file_name = ""

    # flags 
    byte    = 1 << 0
    half    = 1 << 1
    word    = 1 << 2
    double  = 1 << 4
    signed  = 1 << 5

    def __init__(self, test: bool = False):
        if test:
           return 

        ports = [x for x in os.listdir("/dev") if x.startswith("tty.") and "usbmodem" in x]
        assert ports
        self.port_name = f"/dev/{ports[0]}"
        self.file_name = pathlib.Path(__file__).parent / "test.json"

        try:
            self.ser = serial.Serial(self.port_name, 115200)
            self.ser.write()
        except SerialException as e:
            
            raise ServerException(*e)

    def convert_to_bytes(self, data: int, size = 2) -> bytearray:
        if not size in [0,1,2,4]:
            raise ServerException(f"Invalid data size: {size}. Must be [0,1,2,4].")
    
        
        b = bytearray()
        
        mask = 0
        if data < 0:
            mask |= self.signed
        mask |= size

        b.append(mask)

        b += data.to_bytes(size, byteorder='little', signed=data<0)
        return b

    @staticmethod
    def get_size(num: int) -> int:
        #todo i hate this
        if num < 0:
            num *= -1
        if num > (1 << 32):
            return 8
        if num > (1 << 16):
            return 4
        if num > (1 << 8):
            return 2
        return 1
        
        # get the smallest word size that fits the num
        
        # just invert negative numbers?
        # temp = num
        # size = 0
        # while (temp):
        #     temp >>= 8
        #     size += 1

        # return size


    def send(self, data):
        # convert data to format 
        # write to port
        pass

    def recv(self):
        # receive from port
        # convert to data
        pass

class Server(UART):
    pass


class Client(UART):
    pass


def main():
    args = sys.argv[1:]


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
    server = Server()
    # todo

    
