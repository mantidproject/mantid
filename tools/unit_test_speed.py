# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Usage unit_test_speed.py <build-log> <output-csv>

Given a the raw output from a Jenkins build server log this script will output
a CSV file of the speed for each unit test.
"""

import sys

with open(sys.argv[1], "r") as f:
    lines = f.readlines()

lines = filter(lambda x: "....   Passed" in x, lines)
names = [line.split(":")[1].split("....")[0] for line in lines]
times = [line.split("Passed")[-1].split("sec")[0] for line in lines]

with open(sys.argv[2], "w") as f:
    f.write("name, time\n")
    for name, time in zip(names, times):
        f.write(", ".join([name, time]) + "\n")
