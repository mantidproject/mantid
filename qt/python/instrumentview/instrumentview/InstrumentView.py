# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import sys
from qtpy.QtWidgets import QApplication
from .pyvista_test import MainWindow


class InstrumentView:
    def main():
        sys.exit(InstrumentView.start_app_open_window())

    def start_app_open_window():
        app = QApplication(sys.argv)
        window = MainWindow()
        window.show()
        app.exec_()


def open_instrument_view():
    InstrumentView.start_app_open_window()


if __name__ == "__main__":
    InstrumentView.main()
