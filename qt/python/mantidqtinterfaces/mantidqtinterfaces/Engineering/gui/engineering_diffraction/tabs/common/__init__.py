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


def wsname_in_instr_run_ceria_group_ispec_unit_format(ws_name):
    words = ws_name.split("_")
    # fairly length validation method to try and determine with some confidence that
    # filename fits the: 'INSTR_CERIANUM_RUNNUM_GROUP_ispec_UNIT' format
    if words[0] not in INSTRUMENT_DICT.values():
        return False
    if not words[1].isnumeric():
        # expect second word to be a ceria run number so all characters should be numbers
        return False
    if not words[2].isnumeric():
        # expect third word to be an exp run number so all characters should be numbers
        return False
    if not words[4].isnumeric():
        # should be a spectrum index
        return False
    if words[5] not in ("dSpacing", "TOF"):
        # currently supported units for fitting tab
        return False
    return True


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
