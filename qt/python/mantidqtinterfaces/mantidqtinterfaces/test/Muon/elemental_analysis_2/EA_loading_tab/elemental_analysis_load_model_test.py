# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class LoadWidgetModelEATest(unittest.TestCase):
    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = LoadWidgetModel(self.loaded_data, self.context)

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        self.assertEqual(self.model._loaded_data_store.get_data(), None)
        self.assertEqual(self.model._data_context.run_info, [])

    def test_current_runs_and_runs(self):
        self.assertEqual(len(self.model.runs), 0)
        self.model.current_runs = [[1234], [1235]]
        self.assertEqual(len(self.model.runs), 2)
        self.assertEqual(self.model.current_runs, [[1234], [1235]])

    def test_model_is_cleared_correctly(self):
        run = 1237
        grpws = mock.Mock()
        self.model.current_runs = [[run]]
        self.model._loaded_data_store.add_data(run=[run], workspace=grpws)
        self.assertEqual(len(self.model.runs), 1)
        self.assertEqual(self.model._loaded_data_store.num_items(), 1)
        self.model.clear_data()
        self.assertEqual(len(self.model.runs), 0)
        self.assertEqual(self.model._loaded_data_store.num_items(), 0)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
