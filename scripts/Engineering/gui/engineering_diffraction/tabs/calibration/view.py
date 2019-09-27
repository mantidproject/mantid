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
        self.finder_calib.setLabelText("Calibration Sample #")
        self.finder_calib.setInstrumentOverride("ENGINX")

        self.finder_vanadium.setLabelText("Vanadium #")
        self.finder_vanadium.setInstrumentOverride("ENGINX")
        self.setup_tabbing_order()

    def set_on_calibrate_clicked(self, slot):
        self.button_calibrate.clicked.connect(slot)

    def get_vanadium_filename(self):
        return self.finder_vanadium.getFirstFilename()

    def get_calib_filename(self):
        return self.finder_calib.getFirstFilename()

    def get_plot_output(self):
        return self.check_plotOutput.isChecked()

    def setup_tabbing_order(self):
        # TODO
        print()
