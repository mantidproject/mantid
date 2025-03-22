# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.InstrumentView import InstrumentView
import argparse
from pathlib import Path

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Displays the 3D view of an instrument, given a file.")
    parser.add_argument("--file", help="File path", type=str, required=True)
    args = parser.parse_args()
    InstrumentView.main(Path(args.file))
