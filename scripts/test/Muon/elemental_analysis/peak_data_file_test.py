# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import print_function, absolute_import

from qtpy.QtGui import QCloseEvent
import matplotlib
import unittest
import json
import pprint

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from testhelpers import assertRaisesNothing

from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name
from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.multi_plotting_widget import MultiPlotWidget
from MultiPlotting.label import Label
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table import _elements
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import valid_data


class PeakDataFileTest(unittest.TestCase):
    def setUp(self):
        self.file_name = PeriodicTableModel().get_default_peak_data_file()
        with open(self.file_name, 'r') as _file:
            self.data = json.load(_file)
        self.el_names = [data[0] for data in _elements]

    def test_every_peak_is_assigned_to_an_element(self):
        for key in self.data.keys():
            self.assertIn(key, self.el_names)

    def test_every_element_contains_valid_data(self):
        for element, data in self.data.items():
            if not valid_data(data):
                raise Exception("Element '{}' does not contain valid data".format(element))

    def test_some_elements_have_gamma_peaks(self):
        if not any(['Gammas' in data.keys() for _, data in self.data.items()]):
            raise Exception('No element has gamma peaks')

    def test_some_elements_have_electron_peaks(self):
        if not any(['Electrons' in data.keys() for _, data in self.data.items()]):
            raise Exception('No element has electron peaks')

    def test_atomic_number_matches_the_one_in_periodic_table(self):
        for element in _elements:
            from_file = self.data.get(element[0], None)
            if from_file is not None:
                self.assertEqual(element[1], from_file['Z'])

    def test_atomic_mass_matches_the_one_in_periodic_table(self):
        for element in _elements:
            from_file = self.data.get(element[0], None)
            if from_file is not None and from_file['A'] is not None:
                self.assertAlmostEqual(element[5], from_file['A'])

