import numpy as np
import csv
import matplotlib.pyplot as plt
from pathlib import Path

DATA_DIR = Path("../data").absolute().resolve()

# these are all headers (or keys if using dictreader)
stamp    = "Time [s]"
beat     = "Channel 0"
start    = "Channel 1"

pulses = []
stamps = []

total_time = 0
with open(DATA_DIR / "smoothing_test.csv", "r") as f:
    reader = csv.DictReader(f)

    for row in reader:
        # if beat is 1, the signal was high
        # so take the timestamp of each row that has beat = 1
        # and subtract from previous
        if int(row[beat]) != 1:
            continue
        rising = float(row[stamp])
        diff = rising - (stamps[-1] if stamps else 0)
        pulses.append(diff)
        stamps.append(rising)
        if pulses[-1] == np.inf:
            pass
        total_time = float(row[stamp]) # on the final row, this should be the last stamp



# plt.plot(np.arange(0, total_time, step = 10/1000), pulses)  # 10ms precision
plt.plot(stamps, pulses)
plt.xlim(0, total_time)
print(max(pulses))
# plt.ylim(0, max(pulses))
plt.xlabel("Time (ms)")
plt.ylabel("Time since last pulse (ms)")
plt.show()
