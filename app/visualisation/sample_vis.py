from pathlib import Path
from matplotlib import pyplot as plt
from matplotlib.figure import Figure
from matplotlib.axes import Axes
import numpy as np
import json
import os

DATA_DIR = Path("../data").absolute().resolve()


def get_data():
    global DATA_DIR
    sound_files = [x for x in os.listdir(DATA_DIR) if x.endswith(".json")]
    data = {}
    for sf in sound_files:
        # init place to store the sample data
        key = sf.split(".")[0]
        data[key] = {}

        with open(DATA_DIR / sf, "r") as f:
            raw = json.load(f)
            x_points = [x[0] for x in raw]
            y_points = [abs(x[1]) for x in raw]
            data[key]["x"] = x_points
            data[key]["y"] = y_points
            data[key]["sample"] = [x[1] for x in raw]
    return data

def my_key(pair):
    x = pair[0]
    parts = x.split("-")
    ans =  parts[-1] + "".join(parts[0:-1])
    return ans

def visualise_all(data, target: str):

    # params are rows and cols of subplots
    # todo unsure if this is expandable but works for 8 ?
    rows, cols = 2, 4
    fig, axs = plt.subplots(rows, cols)
    fig: Figure
    axs: Axes
    idx = 0

    # sort them into precise and imprecise
    sorted_items = sorted(data.items(), key=my_key)
    for title, points in sorted_items:
        i, j = idx // cols, idx % cols
        axs[i, j].plot(points["x"], points[target], )
        axs[i, j].set_title(title)
        axs[i, j].set_ylim(-4096, 4095)
        idx += 1

    plt.show()

def visualise_diff(data:dict):

    fig, ax = plt.subplots()
    fig: Figure
    ax: Axes

    # input signals
    x_points = []
    
    # average diff between samples
    y_points = []

    sorted_items = sorted(data.items(), key=my_key)
    for title, points in sorted_items:
        # s = "".join([x[0] for x in title.split('-')])
        x_points.append(title) 

        avg = 0
        for i in range(1, len(points["x"])):
            avg += abs(points["x"][i] - points["x"][i-1])
        avg = avg / (len(points["x"]) - 1)
        y_points.append(avg)

    resolution = 10 #ms of mcu
    fs = 78000      # samples / s

    print(f"expected fs: {fs / 1000}ksps, actual: {1 / y_points[1] / resolution:.3f}ksps")
    # x_points.append("expected")
    # y_points.append(1 / fs * 10 * 1000)
    # ax.bar(np.arange(len(x_points)), y_points, label=x_points)

    # ax.set_xticks(np.arange(len(x_points)))
    # ax.set_xticklabels(x_points, rotation = 45)

    # plt.show()



def main():
    target = "y"
    target = "sample"
    data = get_data()
    # visualise_diff(data)
    visualise_all(data, target)

if __name__ == "__main__":
    main()