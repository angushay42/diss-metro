from io import BytesIO
# from server import MockSerial, BytesManager
from collections import deque
import time
import numpy as np
from pathlib import Path
import json



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


DATA_DIR = Path(__file__).parent.parent / "data"


def monotonic_test():
    data = [8, 1, 2, 5, 3, 4, 1]
    expected = 4    # 8, 5, 4, 1
    num_decreasing = 0
    last_number = 0
    last_idx    = 0
    # count how many numbers there are in strictly decreasing order
    for i in range(1, len(data)):
        if data[i] < data[i-1]:
            num_decreasing += 1
        else:
            num_decreasing 
samples = []
with open(DATA_DIR / "finger-constant-precise.json", "r") as f:
    samples = json.load(f)
    samples = [x[1] for x in samples]
i = 0
def get_sample():
    global i
    i += 1
    return samples[(i-1)% len(samples)]

def detect_note():
    # general variables
    bpm = 130
    beats_freq = bpm / 60 
    period = 1/ beats_freq

    # add some beat counts
    start, stop = 0, 3
    start_time = time.time()
    beats = np.arange(start_time+start, start_time+ stop, period)
    beat_idx = 1

    # sample window stuff
    window = 50 / 1000  # in seconds
    sample_rate = 780000    # samples/s

    detections = []

    # outerloop stuff
    time_to_stop = time.time() + stop
    while True:
        # outer loop
        now = time.time()
        if now >= time_to_stop or beat_idx == len(beats):
            break

        start_listen = beats[beat_idx] - (window / 2)
        stop_listen = start_listen + window

        most = 0

        if now >= start_listen and now < stop_listen:
            print("listening")
            sample = get_sample()
            sample = abs(sample)    # get amplitude

            if sample > most:
                # update loudest value
                most = sample
                # mark this as a detection
                detections.append(now)
            # elif sample < most:
    print(detections)




        

    

def random_test_i_forgot():
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
    

if __name__ == "__main__":
    
    detect_note()