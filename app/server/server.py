import serial
from serial import SerialException
from io import BytesIO
import time
import pathlib  
import os
import sys
from collections import deque


class UARTException(BaseException):
    """Base exception for this project."""
    # data: typing.Any

class UART:
    port_name = ""
    file_name = ""
    stream: serial.Serial   = None
    manager: 'BytesManager' = None
    buffer: BytesIO         = None

    def __init__(self, timeout: float, test: bool = False):
        self.timeout = None if timeout <= 0 else timeout
        self.manager = BytesManager()
        self.buffer = BytesIO()
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
            return UARTException(f"Invalid flag {bin(flag):8} with invalid bits {bin(flag >> self.manager.signed):8}")
        
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
            if d == ord('\n'):
                potential = {}
                continue
            if isinstance(self.validate_flag(d), UARTException):
                continue
            if not d in potential:
                potential[d] = 0
            potential[d] += 1
            if potential[d] == 3:
                break
        
        return d

    def detect_packet(self) -> bool:
        # mark all start bytes
            # store upon finding one
        # check for valid packets when stop byte is found
            # if len byte matches length found 
            # if flag matches etc
        # flush buffer
        start_byte = self.manager.start.to_bytes(1, 'little')
        stop_byte = self.manager.stop.to_bytes(1, 'little')

        starts = deque()
        while True:
            # read from input stream
            d = self.stream.read()
            if d == start_byte:
                # store in buffer and mark the start of a potential packet
                self.buffer.write(d)
                starts.append(self.buffer.tell() - 1)
                continue
            # if there's a potential packet, keep storing to buffer
            if starts:
                self.buffer.write(d)
            # if we find a stop byte, check if there is a valid packet
            if d == stop_byte:
                # NOTE: this is the index of the last char, not length.
                maybe_end = self.buffer.tell() - 1
                # for each potential start
                while starts:
                    # check if distance is valid
                    maybe_start = starts.popleft()
                    total_len = abs(maybe_start - maybe_end)
                    # packet sizes
                    if not (BytesManager.packet_min <= total_len + 1 <= BytesManager.packet_max):
                        continue
                    # seek to start + 1 (pointing at flag byte)
                    self.buffer.seek(1,maybe_start)

                    flag = int.from_bytes(self.buffer.read(1), 'little')
                    
                    length = int.from_bytes(self.buffer.read(1), 'little')
                    size = self.manager.get_size_from_flag(flag)
                    if (size * length) != total_len - 4:
                        continue

                return True
    
        
        return False

    def recv(self, strict: bool=False) -> tuple[list[int], str | None] | None:
        """"""
        
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

    packet_min:int = 5  # start|flag|len|data|stop. data in [1, 255]
    packet_max:int = 5 + 254 

    start: int  = ord('{')
    stop: int   = ord('}')

    @classmethod
    def get_size_from_flag(cls, flag):
        if 1 & (flag >> cls.double):
            return 8
        flag &= ~(3 << cls.signed)  # extract size
        return flag

    @classmethod
    def validate_packet(
        cls,
        packet: bytes | bytearray
    ) -> bool:
        

    @classmethod
    def get_flag(
        cls, 
        size: int, 
        is_signed: bool = False, 
        is_float: bool= False) -> int:
        if not size in [1,2,4,8]:
            raise UARTException(f"Invalid size of {size}. Must be in [1,2,4,8].")
        return (
            (1 << (size // 2)) 
            | ((is_signed & 1) << cls.signed)
            | ((is_float & 1) << cls.floating)
        )

    @classmethod
    def get_packet(
        cls,
        size: int,
        data: list | int | float,
        is_signed: bool = False,
        is_float: bool = False
    ) -> bytearray | bytes:
        if not size in [1,2,4,8]:
            raise UARTException(f"Invalid size of {size}. Must be in [1,2,4,8].")
        lenb = 1 if not isinstance(data, list) else len(data)
        b = bytearray(
            # start     flag                                    
            [cls.start, cls.get_flag(size, is_signed, is_float), lenb]
        )
        b += cls.convert_to_bytes(data, size, is_signed)
        b += cls.stop.to_bytes(1, 'little')
        return b
        
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
    pos: int = 0
    size: int
    

    def __init__(self, timeout: float= 0):
        # self.size = size  # todo
        self.io = BytesIO()
        self.timeout = timeout

    def read(self, n: int = 1) -> bytes | None:
        new = self.io.seek(self.idx)
        start = time.time()
        while True:
            if self.timeout != 0 and time.time() - start >= self.timeout:
                return None
            data = self.io.read(n)
            if data:
                break
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

    