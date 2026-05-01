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

    def __init__(self, timeout: float, test: bool = False):
        self.timeout = None if timeout <= 0 else timeout
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
            self.stream.flush()
        except SerialException as e:
            raise UARTException(e)  # todo
        
        self.manager = BytesManager()



    def validate_flag(self, flag: int) -> None | UARTException:
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
            return UARTException(f"Invalid flag {bin(flag):8} with {num_bits} bits. Should have 2.")
        
        if (flag >> (self.manager.signed+1)) != 0:
            return UARTException(f"Invalid flag {bin(flag):8} with invalid bits {bin(flag >> self.server.signed):8}")
        
        return

    def send(self, data):
        # todo validate length is 0-255
        # convert data to format 
        # write to port
        pass

    def get_flag(self, strict: bool = False) -> int:
        """Wait until flag is fond, then return it."""
        potential = {}
        while True:
            d = int.from_bytes(self.stream.read(), 'little')
            if isinstance(self.validate_flag(d), UARTException):
                continue
            if not d in potential:
                potential[d] = 0
            potential[d] += 1
            if potential[d] == 3:
                break
        
        return d

    def recv(self, strict: bool=False) -> tuple[list[int], str | None] | None:
        # todo FLOATING POINT NUMBERS ??
        """Read flag unsigned byte (flag), length unsigned byte (n) and return n bytes (data) and string, if possible."""

        # bug
        """Note: UART is unpredictable so we need to verify messages, not just expect perfect scenarios. """

        flag = self.get_flag()

        b = self.stream.read(1)
        if not b:
            if strict:
                raise UARTException("Length byte not found.")
            else:
                return 
        len = int.from_bytes(b, 'little', signed=False)
        
        
        # extract meta data from flags: Are bytes signed, how big are they?
        signed = bool((flag >> self.manager.signed) & 1)
        size = 1 << int(math.log2(flag & ((1 << self.manager.signed) - 1)))

        data = []
        for i in range(len):
            # read size bytes
            b = self.stream.read(size)
            if not b:
                if strict:
                    raise UARTException(f"Could not read byte {i} of {size} bytes.")
                else:
                    return 
            # format them as specified by flag
            data.append(int.from_bytes(b, 'little', signed=signed))

        # probably a character
        if not signed and size == (1 << self.manager.byte):
            s = "".join([chr(x) for x in data])
            return (data, s)
        
        return (data, None)
    
    def main(self, strict:bool=False):

        try:
            while True:
                data = self.recv(strict)
                if not data:
                    continue
                print(data[0])
        except KeyboardInterrupt:
            pass
        self.shutdown()
    
    def shutdown(self):
        self.stream.close()
        print()
        
class BytesManager:
    byte        = 0
    half        = 1
    word        = 2
    double      = 4
    floating    = 5
    signed      = 6

    @classmethod
    def _convert_many(cls, data: list[int], size: int, signed: bool) -> bytearray:
        if not data:
            raise UARTException(f"Empty list.")
        b = bytearray()

        for d in data:
            b += cls._convert_once(d, size, signed)
        return b 

    @classmethod
    def _convert_once(cls, data: int, size: int, signed: bool) -> bytes:
        try:
            return data.to_bytes(size, byteorder='little', signed=signed)
        except OverflowError as e:
            if (data < 0) != signed:
                raise UARTException(f"Inconsistent polarity for data {data}. Expected {"" if signed else "un"}signed.")
            else:
                raise UARTException(f"Incompatible size: {size} for data {data}.")

    @classmethod
    def convert_to_bytes(
        cls, 
        data: int | list, 
        size = 2, 
        signed: bool = False
    ) -> bytearray | bytes:
        if not size in [1,2,4,8]:
            raise UARTException(f"Invalid data size: {size}. Must be [1,2,4,8].")

        if isinstance(data, list):
            return cls._convert_many(data, size, signed)
            
        if isinstance(data, int):
            return cls._convert_once(data, size, data < 0)        
        
        raise UARTException(f"Invalid type for data: {type(data)}. Should be int or list.")
    
    @classmethod
    def generate_message(cls, data: list | int | float, size: int, signed: bool=False):
        if type(data) not in [list, int, float]:
            raise UARTException(f"Invalid type for data: {type(data)}. Should be int, float or list.")
        
        b = bytearray()
        
        # create flag byte
        flag = 1 << (size // 2)
        flag |= (signed & 1) << cls.signed
        flag |= (isinstance(data, float) & 1) << cls.floating

        b += flag.to_bytes(1, 'little')

        if isinstance(data, list):
            # length byte        
            try:
                b += int.to_bytes(len(data), 'little')
            except OverflowError:
                raise UARTException(f"Data too large: {len(data)}. Must be between 0-255.")
            return b + cls._convert_many(data, size, signed)
        
        b += int.to_bytes(1, 'little')
        return b + cls._convert_once(data, size, signed)

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
    start = time.time()
    server = UART(0)
    server.main()
    end = time.time()
    print(end - start)

    