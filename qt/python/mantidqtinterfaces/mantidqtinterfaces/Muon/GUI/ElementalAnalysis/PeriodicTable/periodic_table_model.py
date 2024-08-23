# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json

from mantid.utils.muon import PEAK_DATA_JSON


class PeriodicTableModel(object):
    def __init__(self):
        self._peak_data = {}
        self._peak_data_file = self.get_default_peak_data_file()
        self.load_peak_data()

    def get_default_peak_data_file(self):
        return PEAK_DATA_JSON

    def load_peak_data(self):
        # using os.path.isfile allows file modifications to take place before opening:
        # we don't want this.
        with open(self._peak_data_file, "r") as f:
            self._peak_data = json.load(f)

    @property
    def peak_data(self):
        return self._peak_data

    @property
    def peak_data_file(self):
        return self._peak_data_file

    @peak_data_file.setter
    def peak_data_file(self, filepath):
        self._peak_data_file = filepath
        self.load_peak_data()
