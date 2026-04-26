import serial
from serial import SerialException
from collections import defaultdict
from io import BytesIO
import math
import time
import pathlib  
import os
import sys
import typing



class UARTException(BaseException):
    """Base exception for this project."""
    # data: typing.Any

class UART:
    port_name = ""
    file_name = ""
    stream: serial.Serial

    byte    = 0
    half    = 1
    word    = 2
    double  = 4
    signed  = 5

    def __init__(self, timeout: float, test: bool = False):
        self.timeout = timeout
        if test:
            return
        

        ports = [x for x in os.listdir("/dev") if x.startswith("tty.") and "usbmodem" in x]
        assert ports
        self.port_name = f"/dev/{ports[0]}"
        self.file_name = pathlib.Path(__file__).parent / "test.json"

        try:
            self.stream = serial.Serial(
                self.port_name, 
                115200,
                timeout=self.timeout
            )
        except SerialException as e:
            
            raise UARTException(*e)

    def convert_to_bytes(self, data: int, size = 2) -> bytearray:
        if not size in [1,2,4,8]:
            raise UARTException(f"Invalid data size: {size}. Must be [1,2,4,8].")
    
        b = bytearray()
        
        mask = 0
        if data < 0:
            mask |= (1 << self.signed)
        mask |= 1 << (size // 2)

        b.append(mask)

        b += data.to_bytes(size, byteorder='little', signed=data<0)
        return b

    @staticmethod
    def get_size(num: int) -> int:
        # todo i hate this
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

    def validate_flag(self, flag: int) -> None:
        """
        Raises UARTException if flag is invalid.
        """
        num_bits = 0
        temp = flag
        while temp:
            num_bits += temp & 1
            temp >>= 1
        # 1 or 2 bits are set
        if not (0 < num_bits <= 2):
            raise UARTException(f"Invalid flag {bin(flag):80} with {num_bits} bits; should have 2.")
        
        if (flag >> (self.signed+1)) != 0:
            raise UARTException(f"Invalid flag {bin(flag):80} with invalid bits {flag >> self.signed}")
        

    def send(self, data):
        # convert data to format 
        # write to port
        pass

    def recv(self) -> tuple[list[int], str | None]:
        """Read flag unsigned byte (flag), length unsigned byte (n) and return n bytes (data) and string, if possible."""
        b = self.stream.read(1)
        if not b:
            raise UARTException("Flag byte not found.")
        flag = int.from_bytes(b, 'little', signed=False)
        
        # validate_flag may throw an exception
        self.validate_flag(flag)

        b = self.stream.read(1)
        if not b:
            raise UARTException("Length byte not found.")
        len = int.from_bytes(b, 'little', signed=False)
        
        # extract meta data from flags: Are bytes signed, how big are they?
        signed = bool((flag >> self.signed) & 1)
        size = flag & ((1 << self.signed) - 1)

        data = []
        for i in range(1, len // size):
            # read size bytes
            b = self.stream.read(size)
            if not b:
                raise UARTException(f"Could not read byte {i} of {size} bytes.")
            # format them as specified by flag
            data.append(int.from_bytes(b, 'little', signed=signed))

        # probably a character
        if not signed and size == self.byte:
            s = "".join([chr(x) for x in data])
            return (data, s)
        
        return (data, None)
        

class MockSerial:
    io: BytesIO
    idx: int = 0    # absolute position to start of next read
    size: int

    def __init__(self):
        # self.size = size  # todo
        self.io = BytesIO()

    def read(self, n: int = 1):
        new = self.io.seek(self.idx)
        data = self.io.read(n)
        self.idx = new    # read might do this already?
        return data

    def write(self, b): # todo typing bytes-like
        n = self.io.write(b)
        return n



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
    # todo
    server = UART()

    
