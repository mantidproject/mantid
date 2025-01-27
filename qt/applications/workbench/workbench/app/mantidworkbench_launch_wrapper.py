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


def launch(args=None):
    if args is None:
        args = sys.argv
    if sys.platform.startswith("linux"):
        command = ["launch_mantidworkbench"] + args[1:]
        subprocess.run(command)
    else:
        main(args[1:])


if __name__ == "__main__":
    sys.exit(launch(sys.argv))
