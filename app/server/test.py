from io import BytesIO
from server import MockSerial, BytesManager
from collections import deque
import time

def main():
    ms = MockSerial()

def detect_packet() -> bool:
        start = time.time()
        # mark all start bytes
            # store upon finding one
        # check for valid packets when stop byte is found
            # if len byte matches length found 
            # if flag matches etc
        # flush buffer
        start_byte = BytesManager.start.to_bytes(1, 'little')
        stop_byte = BytesManager.stop.to_bytes(1, 'little')

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
                    size = BytesManager.get_size_from_flag(flag)
                    if (size * length) != total_len - 4:
                        continue

                return True
    
        
        return False


if __name__ == "__main__":
    buf = BytesIO()

    buf.write(bytearray([ord('{'), 1, 1, 1, ord('{')]))
    start = 0
    end = buf.tell() - 1
    length = (end - start + 1)
    buf.seek(1, start)
    flagb = int.from_bytes(buf.read(1), 'little')
    size = BytesManager.get_size_from_flag(flagb)
    print(f"flag: {flagb}")
    # buf.seek(2, start)
    lenb = int.from_bytes(buf.read(1), 'little')
    print(f"len: {lenb}") 
    print(length == 4 + (size * lenb))
    
    main()