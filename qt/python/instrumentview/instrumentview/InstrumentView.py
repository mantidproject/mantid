# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from mantid.simpleapi import LoadRaw, LoadNexus, LoadEventNexus
from pathlib import Path
from qtpy.QtWidgets import QApplication
import sys


class InstrumentView:
    def main():
        sys.exit(InstrumentView.start_app_open_window())

    def start_app_open_window():
        app = QApplication(sys.argv)
        workspace_path = Path(r"C:\Tutorial\SampleData-ISIS\MAR11060.raw")
        is_event = False
        if is_event:
            ws = LoadEventNexus(str(workspace_path), StoreInADS=False)
        elif workspace_path.suffix == ".nxs":
            ws = LoadNexus(str(workspace_path), StoreInADS=False)
        else:
            ws = LoadRaw(str(workspace_path), StoreInADS=False)
        window = FullInstrumentViewWindow(ws)
        window.show()
        app.exec_()


def open_instrument_view():
    InstrumentView.start_app_open_window()


if __name__ == "__main__":
    InstrumentView.main()
