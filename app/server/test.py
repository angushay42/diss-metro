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
with open(DATA_DIR / "pick-constant-precise.json", "r") as f:
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
            

# https://stackoverflow.com/questions/294468/note-onset-detection
# this link is super helpful. the top answer is very detailed however I think I just need to adjust 
# my method a little bit, and actually it seems like a combination of differernt methods I've tried. Will update git.
def detect_onsets(window_size: int, onset_thresh: int) -> list[tuple[int,int]]:
    onsets = []
    avg = samples[0]

    low_signal_thresh = 100  # roughly what my guitar sits at when I don't play

    n = len(samples)

    # window start, stop, and average
    wstart, wstop = 0, 0
    wsum = samples[0]
    while wstop < n:
        # constantly track average low signal
        if abs(samples[wstop] - avg) < low_signal_thresh:
            avg = ((avg * (wstop)) + samples[wstop]) / max(1, wstop + 1)
        # fill window
        if wstop - wstart < window_size:
            wsum += samples[i]
            wstop += 1
        else:
            wsum += samples[wstop]
            # get average of window
            wavg = wsum / window_size 
            if wavg > avg and wavg > onset_thresh:
                
                onsets.append((wstart, wstop))
            wsum -= samples[wstart]
            
            # move window
            wstart += 1
            wstop += 1
        
      
    return onsets


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
    
    bpm = 120
    # roughly the width of a beat, in ms
    window_size = round(1 / ((bpm / 60) / 1000)) - 200
    window_size = 0 
    min_window = 100
    window_size = max(min_window, window_size)

    onsets = detect_onsets(window_size, 1200)

    plt.stem(x_points, samples, markerfmt=" ")
    starts = [x_points[x[0]] for x in onsets]
    stops = [x_points[x[1]] for x in onsets]
    # plt.vlines(starts, ymin=-4096, ymax=4095, colors="g", alpha=0.4)
    # plt.vlines(stops, ymin=-4096, ymax=4095, colors="r",  alpha=0.4)
    plt.hlines([samples[x[0]] for x in onsets], starts, stops, colors="r" )
    plt.show()

def test_average():
    avg = samples[0]
    avgs = [avg]

    thresh = 100
    for i in range(1, len(samples)):
        if abs(samples[i] - avg) < thresh:

            avg = ((avg * (i)) + samples[i]) / max(1, i + 1)
        avgs.append(avg)

    plt.stem(x_points, samples, markerfmt=" ")
    plt.plot(x_points, avgs, c="r")
    plt.show()

if __name__ == "__main__":
    import sys

    testing = False
    if len(sys.argv) > 1 and sys.argv[1] == "T":
        testing = True
    

    test_onsets()

    

    

    