from pathlib import Path
from matplotlib import pyplot as plt
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
        y_points = [x[1] for x in raw]
        data[key]["x"] = x_points
        data[key]["y"] = y_points

# params are rows and cols of subplots
rows, cols = 2, 2
fig, axs = plt.subplots(rows, cols)

idx = 0
for title, points in data.items():
    i, j = idx // rows, idx % cols
    axs[i, j].plot(points["x"], points["y"])
    axs[i, j].set_title(title)
    axs[i, j].set_ylim(-4096, 4095)
    idx += 1
plt.show()