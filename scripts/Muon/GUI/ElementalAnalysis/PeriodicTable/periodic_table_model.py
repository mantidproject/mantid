# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import print_function, absolute_import

import os
import json

from Muon.GUI import ElementalAnalysis


class PeriodicTableModel(object):
    def __init__(self):
        self._peak_data = {}
        self._peak_data_file = self.get_default_peak_data_file()
        self.load_peak_data()

    def get_default_peak_data_file(self):
        return os.path.join(os.path.dirname(ElementalAnalysis.__file__), "peak_data.json")

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
