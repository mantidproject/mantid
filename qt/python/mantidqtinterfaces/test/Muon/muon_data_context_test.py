# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
import unittest
from mantidqt.utils.qt.testing import start_qapplication

from mantid.api import AnalysisDataService, FileFinder
from unittest import mock

from mantidqtinterfaces.Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from mantidqtinterfaces.Muon.GUI.Common.muon_load_data import MuonLoadData
from mantidqtinterfaces.Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.muon_workspace_wrapper import MuonWorkspaceWrapper


@start_qapplication
class MuonDataContextTest(unittest.TestCase):
    def setUp(self):
        self.filepath = FileFinder.findRuns("EMU00019489.nxs")[0]
        self.load_result, self.run_number, self.filename, _ = load_workspace_from_filename(self.filepath)
        self.loaded_data = MuonLoadData()
        self.context = MuonDataContext(load_data=self.loaded_data)
        self.context.instrument = "EMU"

        self.loaded_data.add_data(workspace=self.load_result, run=[self.run_number], filename=self.filename, instrument="EMU")
        self.context.current_runs = [[self.run_number]]
        self.context.update_current_data()

    def tearDown(self):
        AnalysisDataService.clear()

    def test_setting_current_data_with_a_different_field_sends_message_signal(self):
        self.context.current_data["MainFieldDirection"] = "transverse"
        self.context.message_notifier.notify_subscribers = mock.MagicMock()

        self.context.update_current_data()

        self.context.message_notifier.notify_subscribers.assert_called_once_with(
            "MainFieldDirection has changed between data sets, click default to reset grouping if required"
        )
        self.context.current_data["MainFieldDirection"] = "longitudinal"

    def test_that_setting_current_runs_with_mixture_of_transverse_and_longitudanal_runs_raises_warning(self):
        loaded_data = copy.copy(self.context.current_data)
        self.context.message_notifier.notify_subscribers = mock.MagicMock()
        loaded_data["MainFieldDirection"] = "transverse"

        self.loaded_data.add_data(workspace=loaded_data, run=[1], filename="filename", instrument="EMU")

        self.context.current_runs = [[19489], [1]]

        self.context.message_notifier.notify_subscribers.assert_called_once_with(
            "MainFieldDirection changes within current run set:\ntransverse field runs 1\nlongitudinal field runs 19489\n"
        )

    def test_is_data_loaded_returns_true_if_data_loaded(self):
        self.assertTrue(self.context.is_data_loaded())

    def test_is_data_loaded_returns_false_if_no_data_loaded(self):
        self.context._loaded_data.clear()
        self.assertFalse(self.context.is_data_loaded())

    def test_current_filenames_returns_path_to_current_file(self):
        self.assertEqual(self.context.current_filenames, [self.filepath])

    def test_current_runs_set_correctly(self):
        self.assertEqual(self.context.current_runs, [[19489]])

    def test_current_workspaces_returns_correctly(self):
        self.assertEqual(self.context.current_workspaces, [self.load_result])

    def test_get_loaded_data_for_run_returns_correctly(self):
        self.assertEqual(self.context.get_loaded_data_for_run([19489]), self.load_result)

    def test_num_detectors_returns_correctly(self):
        self.assertEqual(self.context.num_detectors, 96)

    def test_num_periods_returns_correctly(self):
        self.assertEqual(self.context.num_periods([19489]), 1)

    def test_main_field_direction_returns_correctly(self):
        self.assertEqual(self.context.main_field_direction, "Longitudinal")

    def test_dead_time_table_returns_correctly(self):
        self.assertEqual(self.context.dead_time_table, self.load_result["DeadTimeTable"])

    def test_return_sample_log_returns_correctly(self):
        self.assertEqual(self.context.get_sample_log("goodfrm").value, 31369.0)

    def test_that_is_multi_period_returns_false_for_single_period_data(self):
        is_multi_period = self.context.is_multi_period()

        self.assertTrue(not is_multi_period)

    def test_that_any_multi_period_data_will_mark_everything_as_multiperiod(self):
        multi_period_worspace_list = [MuonWorkspaceWrapper(f"raw_data_{period_index + 1}") for period_index in range(4)]
        load_result = {"OutputWorkspace": multi_period_worspace_list}
        self.loaded_data.add_data(workspace=load_result, run=[84447], filename="workspace_filename", instrument="EMU")
        self.context._current_runs = [[84447], [19489]]

        is_multi_period = self.context.is_multi_period()

        self.assertTrue(is_multi_period)


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
