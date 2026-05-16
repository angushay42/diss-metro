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
def detect_onsets(
        target: list[int], 
        window_size: int, 
        onset_thresh: int
    ) -> list[tuple[int,int]]:
    """Detects onsets on a given target range of samples"""
    onsets = []
    avg = target[0]

    low_signal_thresh = 80  # roughly what my guitar sits at when I don't play
    n = len(target)

    # window start, stop, and average
    wstart, wstop = 0, 0
    wsum = 0
    while wstop < n:
        # constantly track average low signal
        if abs(target[wstop] - avg) < low_signal_thresh:
            avg = ((avg * (wstop)) + target[wstop]) / max(1, wstop + 1)
        # fill window
        if wstop - wstart < window_size:
            wsum += target[wstop]
            wstop += 1
        else:
            wsum += target[wstop]
            # get average of window
            wavg = wsum / window_size 
            if wavg > avg and wavg > onset_thresh:
                onsets.append((wstart, wstop))
            wsum -= target[wstart]
            
            # move window
            wstart += 1
            wstop += 1
        
      
    return onsets


def compress(x, param:float):
    sign = -1 if x < 0 else 1
    norm = abs(x / 4095)
    norm = 1 - pow(1 - norm, param)
    return 4095 * norm * sign

def log_compress(x, delta, r):
    if x <= delta:
        return x
    return pow(delta, 1-(1/r)) * pow(x, 1/r)
    
def test_detections():
    detections = detect_note()
    
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
    window_size = round(1 / ((bpm / 60) / 1000))

    # distance between samples is roughly 3.3ms, so ideally this would work but needs to be scaled down
    window_size /= 3.3
    window_size /= 2

    min_window = 50
    window_size = max(min_window, window_size)

    delta, r, param = 1500, 2, 2.3
    target = samples
    target = [log_compress(x, delta, r) for x in target]
    onsets = detect_onsets(target, min_window, 900)

    plt.stem(x_points, target, markerfmt=" ")
    starts = [x_points[x[0]] for x in onsets]
    stops = [x_points[x[1]] for x in onsets]
    # plt.vlines(starts, ymin=-4096, ymax=4095, colors="g", alpha=0.4)
    # plt.vlines(stops, ymin=-4096, ymax=4095, colors="r",  alpha=0.4)
    plt.hlines([target[x[0]] for x in onsets], starts, stops, colors="r")

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

def test_compression():
    
    target = samples
    # from https://stackoverflow.com/questions/294468/note-onset-detection#:~:text=Update%3A%20here-,is,-a%20version%20of
    param = 1.5
    lineard = [compress(x, param) for x in target]

    logd = [log_compress(x, 2048, 3) for x in target]

    fig, axs = plt.subplots(3)

    bits = [[lineard, "linear", "r"], [target, "original", "b"], [logd, "logarithmic", "g"]]

    # stem returnes marker, stem, base
    for i, ax in enumerate(axs):
        bit = bits[i]
        m,s,b = ax.stem(x_points, bit[0], markerfmt=" ", label=bit[1])
        s.set_color(bit[2])
        ax.set_ylim((0, 4095))
        ax.legend()

    plt.legend()
    plt.show()

def main():
    import sys

    testing = False
    if len(sys.argv) > 1 and sys.argv[1] == "T":
        testing = True
    
    # test_compression()
    test_onsets()

if __name__ == "__main__":
    main()
    