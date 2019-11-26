# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)


class CalibrationInfo(object):
    """
    Keeps track of the parameters that went into a calibration created by the engineering diffraction GUI.
    """
    def __init__(self, vanadium_path=None, ceria_path=None, instrument=None):
        self.vanadium_path = vanadium_path
        self.ceria_path = ceria_path
        self.instrument = instrument

    def set_calibration(self, vanadium_path, ceria_path, instrument):
        """
        Set the values of the calibration. requires a complete set of calibration info to be supplied.
        :param vanadium_path: Path to the vanadium data file used in the calibration.
        :param ceria_path: Path to the ceria data file used.
        :param instrument: String defining the instrument the data came from.
        """
        self.vanadium_path = vanadium_path
        self.ceria_path = ceria_path
        self.instrument = instrument

    def get_vanadium(self):
        return self.vanadium_path

    def get_ceria(self):
        return self.ceria_path

    def get_instrument(self):
        return self.instrument

    def clear(self):
        self.vanadium_path = None
        self.ceria_path = None
        self.instrument = None

    def is_valid(self):
        return True if self.vanadium_path and self.ceria_path and self.instrument else False

