# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from mantid.simpleapi import Load
from pathlib import Path
from qtpy.QtWidgets import QApplication
import sys


class InstrumentView:
    def main(file_path: Path):
        sys.exit(InstrumentView.start_app_open_window(file_path))

    def start_app_open_window(file_path: Path):
        app = QApplication(sys.argv)
        ws = Load(str(file_path), StoreInADS=False)
        window = FullInstrumentViewWindow(ws)
        window.show()
        app.exec_()


def open_instrument_view(file_path: str):
    InstrumentView.start_app_open_window(file_path)
