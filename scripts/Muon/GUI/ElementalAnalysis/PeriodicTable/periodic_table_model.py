from __future__ import print_function, absolute_import

import os
import json

from Muon.GUI.Common import message_box
from Muon.GUI import ElementalAnalysis


class PeriodicTableModel(object):
    def __init__(self):
        self._peak_data = {}
        self._peak_data_file = os.path.join(
            os.path.dirname(
                ElementalAnalysis.__file__),
            "peak_data.json")
        self.load_peak_data()

    def load_peak_data(self):
        # using os.path.isfile allows file modifications to take place before opening:
        # we don't want this.
        try:
            with open(self._peak_data_file, "r") as f:
                self._peak_data = json.load(f)
        except Exception as error:
            message_box.warning(error)

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
