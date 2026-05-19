from io import BytesIO
# from server import MockSerial, BytesManager
from collections import deque
import time
import numpy as np
from pathlib import Path
import json
import matplotlib.pyplot as plt
from matplotlib.figure import Figure
from matplotlib.axes import Axes
from scipy.signal import butter, filtfilt

DATA_DIR = Path(__file__).parent.parent / "data"

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

# ================ pre-processing function ================== #
def compress(x, param:float):
    sign = -1 if x < 0 else 1
    norm = abs(x / 4095)
    norm = 1 - pow(1 - norm, param)
    return 4095 * norm * sign

def log_compress(x, delta, r):
    if x <= delta:
        return x
    return pow(delta, 1-(1/r)) * pow(x, 1/r)

def lowpass(target, dt, rc):
    """ From wiki: https://en.wikipedia.org/wiki/Low-pass_filter#:~:text=//%20Return%20RC%20low,1%5D%0A%20%20%20%20return%20y
    - target samples
    - dt time difference between each sample
    - RC is a time constant, equal to the reciprocal of 1/(2PI*cutoff)
    """
    assert len(target) > 1
    y = [0]*len(target)
    alpha = dt / (rc + dt)
    y[0] = target[0]*alpha
    for i in range(1, len(target)):
        y[i] = y[i-1] + alpha * (target[i] - y[i-1])

    return y

# ================ plotting helpers ================== #
def plot_line(ax:Axes, x, y, color, label):
    ax.plot(x,y,color=color, label=label)
    return ax

def plot_stem(ax: Axes, x, y, color, label):
    lines = ax.stem(x,y, markerfmt=' ', label=label)
    lines[1].set_color(color)
    return ax

def plot_onsets(ax:Axes, target, onsets, colors="r"):
    heights = [target[x[0]] for x in onsets]
    starts = [x_points[x[0]] for x in onsets]
    stops = [x_points[x[1]] for x in onsets]
    ax = plot_hlines(ax, heights, starts, stops, colors=colors)
    return ax

def plot_hlines(ax:Axes, y, start, stop, colors="r", label=""):
    ax.hlines(y, start, stop, colors=colors, label=label)
    return ax


# ================ test functions ================== #
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

    window_size = 50
    thresh = 900    # amplitude in LSB

    # test no processing, compressed, filtered.
    inputs = []
    labels = []
    colors = ["r"]
    target = samples
    inputs.append(target)
    labels.append("raw")

    # compressed input data
    delta, r, param = 1500, 2, 2.3

    compressed = [log_compress(x, delta, r) for x in target]
    inputs.append(compressed)
    labels.append("compressed")


    # low pass filtered input data
    cutoff = 50    # hz
    dt = 3.3 / 1000 # seconds
    rc = 1 / (2 * np.pi * cutoff)
    filtered = lowpass(target, dt, rc)
    inputs.append(filtered)
    labels.append("filtered")


    double = lowpass(compressed, dt, rc)
    inputs.append(double)
    labels.append("compressed then filtered")

    double2 = [log_compress(x, delta, r) for x in filtered]
    inputs.append(double2)
    labels.append("filtered then compressed")

    smoothed = [(sum(target[i-1:i+2])/3) if 1 <= i < len(target) - 1 else target[i] for i in range(len(target))]
    inputs.append(smoothed)
    labels.append("smoothed")

    # plot raw input data
    fig, axs = plt.subplots(len(inputs))
    axs: list[Axes]
    for i, ax in enumerate(axs):
        target = samples
        plot_stem(ax, x_points, inputs[i], "teal", label=labels[i])

        onsets = detect_onsets(inputs[i], window_size, thresh)
        ax = plot_onsets(ax, inputs[i], onsets, colors=colors[i % len(colors)])
        ax.set_label("")
        ax.legend()
  

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


    colours = ["r", "g", "b"]
    delta_values = [2048]
    r_values = [1, 2, 3, 5, 8, 10]

    fig, axs = plt.subplots(max(len(r_values), len(delta_values)))
    # stem returnes marker, stem, base
    for i, ax in enumerate(axs):
        d = delta_values[i % len(delta_values)]
        r = r_values[i % len(r_values)]

        compressed = [log_compress(x, d, r) for x in target]
        m,s,b = ax.stem(x_points, compressed , markerfmt=" ", label=f"d: {d}, r: {r}")
        s.set_color(colours[i % len(colours)])
        ax.set_ylim((0, 4095))
        ax.legend()

    plt.legend()
    plt.show()

def test_lowpass():
    target = samples
    # dt = 3.3 / 1000 # in seconds
    # rcs = [i * dt for i in range(5)]

    ##  wiki lowpass with different time constants
    # num_plots = len(rcs) + 2    # 1 for orig, 1 for scipy
    # colors = ['r', 'b']
    # fig, axs = plt.subplots(2)
    # fig: Figure
    # axs: list[Axes]
    # # for i in range(num_plots - 1):
    # #     ax = axs[i]
    # #     if i == 0:
    # #         m, s, b = ax.stem(x_points, target, markerfmt=" ")
    # #         s.set_color("g")
    # #         ax.set_ylim(0, 4095)
    # #         continue
    # #     filtered = lowpass(target, dt, rcs[i - 1])
    # #     color = colors[i % len(colors)]
    # #     m,s,b = ax.stem(x_points, filtered, markerfmt=" ", label=f"RC = {rcs[i-1]}")
    # #     s.set_color(color)
    # #     ax.legend()

    
    fig, (ax1, ax2) = plt.subplots(2)
    ax1 = plot_line(ax1, x_points, target, "teal", "original")
    ax1: Axes
    ax2: Axes
    # cutoff frequency randomly selected
    cutoff = 100        # hz
    # average distance of samples from test data set
    dt = 3.3 / 1000     # in seconds
    # given from wikipedia.
    rc = 1 / (2 * np.pi * cutoff)

    # given by example code found from https://medium.com/@ChanakaDev/low-pass-high-pass-and-band-pass-filters-with-scipy-python-a87b2332ce25
    order = 4
    fs = 1 / dt
    nyq = 0.5 * fs  # nyquist freq is half sampling rate
    
    # time markers for different functions
    starts, ends = [], []

    # start timer
    starts.append(time.time())

    # create a filter
    b, a = butter(order, cutoff / nyq, btype='lowpass')
    # apply it forwards and backwards
    filtered = filtfilt(b, a, target)
    
    # mark end
    ends.append(time.time())

    # plot butter filter
    ax1 = plot_line(ax1, x_points, filtered, "red", "SciPy Butter method")
    
    # simple wiki iir filter
    starts.append(time.time())
    filtered = lowpass(target, dt, rc)
    ends.append(time.time())

    # plot simple
    ax1 = plot_line(ax1, x_points, filtered, "green", "Wiki Simple IIR method")

    # plot timings
    ax2.bar(np.arange(len(starts)), [abs(starts[i] - ends[i]) for i in range(len(starts))])
    ax2.set_xticks(np.arange(len(starts)), ["butter", "wiki"])
    ax1.legend()
    plt.show()

def test_smooth():
    fig, axs = plt.subplots(2)
    target = samples
    plot_stem(axs[0], x_points, target, "teal", "base")
    plot_stem(axs[1], x_points, target, "teal", "base")

    smoothed = [(sum(target[i-1:i+2])/3) if 1 <= i < len(target) - 1 else target[i] for i in range(len(target))]
    plot_stem(axs[0], x_points, smoothed, "red", "smoothed")
    axs[0].legend()
    
    fc = 150
    dt, rc = 3.3 / 1000, 1 / (2*np.pi*fc)
    plot_stem(axs[1], x_points, lowpass(target, dt, rc), "red", "filtered")
    axs[1].legend()
    plt.show()

def main():
    import sys

    testing = False
    if len(sys.argv) > 1 and sys.argv[1] == "T":
        testing = True
    
    # fig, ax = plt.subplots()
    # target = y_points
    # plot_stem(ax, x_points, target, "teal", "original")
    # ax.legend()
    # plt.show()
    # test_lowpass()
    # test_compression()
    test_onsets()
    # test_smooth()

if __name__ == "__main__":
    main()
    