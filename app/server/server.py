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
    stream: serial.Serial   = None
    manager: 'BytesManager' = None

    byte        = 0
    half        = 1
    word        = 2
    double      = 4
    floating    = 5
    signed      = 6

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
            raise UARTException(e)  # todo
        
        self.manager = BytesManager()

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
            raise UARTException(f"Invalid flag {bin(flag):8} with {num_bits} bits; should have 2.")
        
        if (flag >> (self.signed+1)) != 0:
            raise UARTException(f"Invalid flag {bin(flag):8} with invalid bits {bin(flag >> self.signed):8}")
        

    def send(self, data):
        # todo validate length is 0-255
        # convert data to format 
        # write to port
        pass

    def recv(self) -> tuple[list[int], str | None]:
        """Read flag unsigned byte (flag), length unsigned byte (n) and return n bytes (data) and string, if possible."""

        # todo FLOATING POINT NUMBERS
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
        for i in range(len // size):
            # read size bytes
            b = self.stream.read(size)
            if not b:
                raise UARTException(f"Could not read byte {i} of {size} bytes.")
            # format them as specified by flag
            data.append(int.from_bytes(b, 'little', signed=signed))

        # probably a character
        if not signed and size == (1 << self.byte):
            s = "".join([chr(x) for x in data])
            return (data, s)
        
        return (data, None)
        
class BytesManager:

    @classmethod
    def _convert_many(cls, data, size, signed):

    

    @classmethod
    def _convert_once(cls, data, size, signed):

    

    @classmethod
    def convert_to_bytes(cls, data: int | list, size = 2, signed: bool = False) -> bytearray:
        if not size in [1,2,4,8]:
            raise UARTException(f"Invalid data size: {size}. Must be [1,2,4,8].")
    
        if isinstance(data, list):
            
            raise UARTException(f"Empty list.")
        
        if isinstance(data, int):
            signed = data < 0

        b = bytearray()
        try:
            b += data.to_bytes(size, byteorder='little', signed=signed)
        except OverflowError as e:
            raise UARTException(f"Incompatible size: {size} for data {data}.")
        return b
    

    @classmethod
    def get_size(cls, num: int) -> int:
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


class MockSerial:
    io: BytesIO
    idx: int = 0    # absolute position to start of next read
    size: int

    def __init__(self):
        # self.size = size  # todo
        self.io = BytesIO()

    def read(self, n: int = 1) -> bytes | bytearray:
        new = self.io.seek(self.idx)
        data = self.io.read(n)
        self.idx = self.io.tell()
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

    
