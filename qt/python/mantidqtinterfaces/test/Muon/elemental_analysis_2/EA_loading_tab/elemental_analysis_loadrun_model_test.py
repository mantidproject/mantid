# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.context.data_context import RunObject
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models import LoadRunWidgetModel
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class LoadRunWidgetModelEATest(unittest.TestCase):
    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = LoadRunWidgetModel(self.loaded_data, self.context)

    def test_model_initialized_with_empty_lists_of_loaded_data(self):
        self.assertEqual(self.model._current_run, None)
        self.assertEqual(self.model._directory, "")
        self.assertEqual(self.model._runs, [])
        self.assertEqual(self.model._loaded_data_store.get_data(), None)
        self.assertEqual(self.model._data_context.run_info, [])

    def test_executing_load_without_runs_does_nothing(self):
        self.model.execute()
        self.assertEqual(self.model._current_run, None)
        self.assertEqual(self.model._directory, "")
        self.assertEqual(self.model._runs, [])
        self.assertEqual(self.model._loaded_data_store.get_data(), None)
        self.assertEqual(self.model._data_context.run_info, [])

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models.LoadElementalAnalysisData')
    def test_execute_successfully_loads_valid_run_single(self, load_ea_data_mock):
        # Mock the load algorithm
        grpws = mock.Mock()
        grpws.getNames.return_value = ['1234; Detector 1', '1234; Detector 3']
        mocked_path = mock.NonCallableMock()
        load_return_vals = grpws, mocked_path

        load_ea_data_mock.return_value = load_return_vals

        self.model._runs = [1234]
        self.model.execute()
        self.assertEqual(self.model._directory, mocked_path)
        run_info = self.model._data_context.run_info[0]
        self.assertIsInstance(run_info, RunObject)
        self.assertEqual(run_info._run_number, 1234)
        self.assertEqual(run_info._detectors, ["Detector 1", "Detector 3"])
        self.assertEqual(run_info._groupworkspace, grpws)
        self.assertTrue(len(self.model.loaded_runs) > 0)

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis2.load_widget.load_models.LoadElementalAnalysisData')
    def test_model_is_cleared_correctly(self, load_ea_data_mock):
        grpws = mock.Mock()
        grpws.getNames.return_value = ['1234; Detector 1', '1234; Detector 3']
        mocked_path = mock.NonCallableMock()
        load_return_vals = grpws, mocked_path

        load_ea_data_mock.return_value = load_return_vals

        self.model._runs = [1234, 1255]
        self.model.execute()
        self.assertEqual(len(self.model.loaded_runs), 2)
        self.model.clear_loaded_data()
        self.assertEqual(self.model.loaded_runs, [])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
