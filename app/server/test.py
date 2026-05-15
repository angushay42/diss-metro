from io import BytesIO
# from server import MockSerial, BytesManager
from collections import deque
import time
import numpy as np
from pathlib import Path
import json
import matplotlib.pyplot as plt


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
x_points, y_points = [], []
samples = []
with open(DATA_DIR / "finger-constant-precise.json", "r") as f:
    samples = json.load(f)
    x_points = [x[0] for x in samples]
    y_points = [x[1] for x in samples]
    samples = [abs(x) for x in y_points]

i = 0
def get_sample():
    global i
    i += 1
    return samples[(i-1)% len(samples)]

def get_time():
    return time.time()


def detect_note(testing: bool):
    # general variables
    bpm = 130
    beats_freq = bpm / 60 
    period = 1/ beats_freq

    # add some beat counts
    start, stop = 0, 3
    start_time = get_time()
    beats = np.arange(start_time + start, start_time + stop, period)
    beat_idx = 1

    # sample window stuff
    window = 10 / 1000  # in seconds
    sample_rate = 780000    # samples/s

    detections = []

    loudness_thresh = 100   # 100 * LSB
    detection_thresh = 2    # num samples decreasing...

    i = 0
    n = len(samples)
    while i < n:
        count = 0
        if samples[i] > loudness_thresh:

            # iterate while values are increasing
            # j is next sample
            count = 0
            j = i + 1
            while j < n and samples[i] < samples[j]:
                count += 1
                j+= 1
            print(f"after increasing... i: {i}, j: {j}, count: {count}")
            # i should now point to a potential start
            i = j
            j = i + 1
            while j < n and samples[i] > samples[j]:
                j += 1
                count += 1
            print(f"after decreasing... i: {i}, j: {j}, count: {count}")
            if count > detection_thresh:
                detections.append((i, j))
            i = j
        else:
            i += 1

    return detections
            

def detect_onsets(onset_thresh):
    onsets = []
    
    peak_thresh = 50
    low_signal_thresh = 100  # roughly what my guitar sits at when I don't play

    # from 1 to n-1
    i, n = 1, len(samples)
    while i < n-1:
        j = i+1 
        # if near silence, prepare for note
        if (samples[i-1] <= low_signal_thresh):
            diff = samples[j] - samples[i]
            if diff > 0 and diff > onset_thresh:
                onsets.append(x_points[i])
        i += 1
      
    return onsets

# pseudo code:
"""
mark likely onsets
"""

        

    

def random_test_i_forgot():
    buf = BytesIO()

    buf.write(bytearray([ord('{'), 1, 1, 1, ord('{')]))
    start = 0
    end = buf.tell() - 1
    length = (end - start + 1)
    buf.seek(1, start)
    flagb = int.from_bytes(buf.read(1), 'little')
    size = 1
    print(f"flag: {flagb}")
    # buf.seek(2, start)
    lenb = int.from_bytes(buf.read(1), 'little')
    print(f"len: {lenb}") 
    print(length == 4 + (size * lenb))
    
def test_detections():
    detections = detect_note(testing)
    
    # # each index is a sample so multiply that by the period ?
    # resolution = 10     # of mcu
    # sample_rate = 78    # samples/ms
    # period = 1 / sample_rate * resolution
    # start = min(x_points)
    
    # starts = [start + (x[0] * period) for x in detections]
    # stops = [start + (x[1] * period) for x in detections]
    
    lims = (min(y_points)-50, max(y_points) + 50)

    # scale = 100  #ms
    # scaled_x = [x * scale for x in x_points]
    starts = [x_points[x[0]] for x in detections]
    stops = [x_points[x[1]-1] for x in detections]

    # plt.plot(x_points, y_points, color="orange")  # orange for actual data
    plt.plot(x_points, samples, color="b")  # blue for samples

    plt.vlines(starts, ymax=lims[1], ymin=lims[0], colors='r')
    plt.vlines(stops, ymax=lims[1], ymin=lims[0], colors='g')
    plt.show()

def test_onsets():
    # LSB = 0.6mV
    onsets = detect_onsets(500)

    plt.plot(x_points, samples)
    
    plt.vlines(onsets, ymin=-4096, ymax=4095, colors="g", alpha=0.2)
    plt.show()

def test_average():
    avgs = []
    count = 0
    for i in range(len(samples)):
        count += samples[i]
        avgs.append(count / (i+1))

    plt.stem(x_points, samples, markerfmt=" ")
    plt.plot(x_points, avgs, c="r")
    plt.show()

if __name__ == "__main__":
    import sys

    testing = False
    if len(sys.argv) > 1 and sys.argv[1] == "T":
        testing = True
    
    test_average()
    

    

    

    