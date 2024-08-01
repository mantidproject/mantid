# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import subprocess
import sys

from workbench.app.main import main


def launch():
    if sys.platform.startswith("linux"):
        subprocess.run(["workbench_launcher_jemalloc"])
    else:
        main()


if __name__ == "__main__":
    launch()
