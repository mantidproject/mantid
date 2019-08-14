# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function

import unittest

from mantid.py3compat import mock
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel


class PeriodicTableModelTest(unittest.TestCase):
    def setUp(self):
        self.model = PeriodicTableModel()
        self.model.load_peak_data = mock.Mock()

    def test_peak_data_file_setter(self):
        self.model.peak_data_file = mock.Mock()
        assert self.model.load_peak_data.call_count == 1

    def test_peak_data_file_getter(self):
        m = mock.Mock()
        self.model.peak_data_file = m
        assert self.model.peak_data_file == m

    def test_data_loaded_with_stock_file(self):
        # loads data before load_peak_data is mocked
        assert self.model.peak_data["H"]["Z"] == 1
        self.assertRaises(KeyError, lambda x: self.model.peak_data[x], "Cf")


if __name__ == "__main__":
    unittest.main()
