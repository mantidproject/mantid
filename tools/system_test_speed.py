# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Usage system_test_speed.py <build-log> <output-csv>

Given a the raw output from a Jenkins build server log this script will output
a CSV file of the speed & memory for each system test.
"""

import sys

with open(sys.argv[1], "r") as f:
    lines = f.readlines()

lines = filter(lambda x: "RESULT|" in x or ": Executing" in x, lines)

# Strip out tests that did not run
# look ahead and remove tests that have no results
idxs = []
for i, (x, y) in enumerate(zip(lines, lines[1::])):
    if "Executing" in x and "Executing" in y:
        idxs.append(i)
lines = [i for j, i in enumerate(lines) if j not in idxs]

with open(sys.argv[2], "w") as f:
    f.write("name, time, memory\n")
    for name, time, memory in zip(lines[::3], lines[1::3], lines[2::3]):
        name = name.split("Executing")[-1].strip()
        time = time.split("|1")[-1].strip()
        memory = memory.split("|")[-1].strip()
        f.write(", ".join([name, time, memory]) + "\n")
