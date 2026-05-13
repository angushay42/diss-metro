from pathlib import Path
from matplotlib import pyplot as plt
from matplotlib.figure import Figure
from matplotlib.axes import Axes
import numpy as np
import json
import os

DATA_DIR = Path("../data").absolute().resolve()

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

# params are rows and cols of subplots
# todo unsure if this is expandable but works for 8 ?
rows, cols = 2, 4
fig, axs = plt.subplots(rows, cols)
fig: Figure
axs: Axes
idx = 0

def my_key(pair):
    x = pair[0]
    parts = x.split("-")
    ans =  parts[-1] + "".join(parts[0:-1])
    return ans


# sort them into precise and imprecise
sorted_items = sorted(data.items(), key=my_key)
for title, points in sorted_items:
    i, j = idx // cols, idx % cols
    axs[i, j].plot(points["x"], points["y"], )
    axs[i, j].set_title(title)
    axs[i, j].set_ylim(-4096, 4095)
    idx += 1
plt.show()