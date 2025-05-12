# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
import os
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
        if sys.platform.startswith("win"):
            # Windows conda installs require us to set the QT_PLUGIN_PATH variable.
            if "CONDA_PREFIX" in os.environ:
                os.environ["QT_PLUGIN_PATH"] = f"{os.environ.get('CONDA_PREFIX')}\\Library\\plugins"
        main(args[1:])


if __name__ == "__main__":
    sys.exit(launch(sys.argv))
