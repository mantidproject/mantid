# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Holds some common constants across all tabs.
"""

from qtpy.QtWidgets import QMessageBox
from mantidqt.utils.observer_pattern import Observer

# Dictionary of indexes for instruments.
INSTRUMENT_DICT = {0: "ENGINX", 1: "IMAT"}


def create_error_message(parent, message):
    QMessageBox.warning(parent, "Engineering Diffraction - Error", str(message))


class CalibrationObserver(Observer):
    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, calibration):
        self.outer.update_calibration(calibration)


class SavedirObserver(Observer):
    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, savedir):
        self.outer.update_savedir(savedir)


class FitObserver(Observer):
    def __init__(self, outer):
        Observer.__init__(self)
        self.outer = outer

    def update(self, observable, fitres):
        self.outer.update_fitres(fitres)
