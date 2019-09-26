# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
from qtpy import QtWidgets
from mantidqt.widgets.filefinder import FileFinder

from mantidqt.utils.qt import load_ui

Ui_calib, _ = load_ui(__file__, "calibration_tab.ui")


class CalibrationView(QtWidgets.QWidget, Ui_calib):
    def __init__(self, parent=None):
        super(CalibrationView, self).__init__(parent)
        self.setupUi(self)

    def on_browse_vanadium_clicked(self, slot):
        self.button_browseVanadium.clicked.connect(slot)


    def on_browse_calibration_clicked(self, slot):
        self.button_browseCalib.clicked.connect(slot)

    def on_calibrate_clicked(self, slot):
        self.button_calibrate.clicked.connect(slot)
