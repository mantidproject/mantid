# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from instrumentview.FullInstrumentViewWindow import FullInstrumentViewWindow
from instrumentview.FullInstrumentViewModel import FullInstrumentViewModel
from instrumentview.FullInstrumentViewPresenter import FullInstrumentViewPresenter
from mantid.simpleapi import Load
from mantid.kernel import logger
from pathlib import Path
from qtpy.QtWidgets import QApplication
from qtpy.QtGui import QIcon
import sys
import os


class InstrumentView:
    """Show the Instrument View in a separate window"""

    def main(file_path: Path) -> None:
        sys.exit(InstrumentView.start_app_open_window(file_path))

    def start_app_open_window(file_path: Path) -> None:
        """Load the given file, then open the Instrument View in a separate window with that workspace displayed"""
        app = QApplication(sys.argv)
        ws = Load(str(file_path))

        if (
            not ws.getInstrument()
            or not ws.getInstrument().getName()
            or not ws.getAxis(1).isSpectra()
            or (ws.detectorInfo().detectorIDs().size == 0)
        ):
            logger.error(f"Could not open instrument for {file_path}. Check that instrument and detectors are present in the workspace.")
            return

        model = FullInstrumentViewModel(ws)
        window = FullInstrumentViewWindow()
        FullInstrumentViewPresenter(window, model)
        current_dir = os.path.dirname(__file__)
        app.setWindowIcon(QIcon(f"{current_dir}/mantidplot.png"))
        window.show()
        app.exec_()
