# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import AnalysisDataService, FileFinder, FrameworkManager

from Muon.GUI.Common.corrections_tab_widget.corrections_model import CorrectionsModel
from Muon.GUI.Common.muon_diff import MuonDiff
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.utilities.load_utils import load_workspace_from_filename


class CorrectionsModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.filepath = FileFinder.findRuns('EMU00019489.nxs')[0]
        self.load_result, self.run_number, self.filename, _ = load_workspace_from_filename(self.filepath)

        self.context = setup_context()
        self.context.gui_context.update({'RebinType': 'None'})
        self.model = CorrectionsModel(self.context)
        self.runs = [[84447], [84448], [84449]]
        self.coadd_runs = [[84447, 84448, 84449]]

        self.context.data_context.instrument = 'EMU'
        self.context.data_context._loaded_data.add_data(workspace=self.load_result, run=[self.run_number],
                                                        filename=self.filename, instrument='EMU')
        self.context.data_context.current_runs = [[self.run_number]]
        self.context.data_context.update_current_data()
        self.context.group_pair_context.reset_group_and_pairs_to_default(
            self.load_result['OutputWorkspace'][0].workspace, 'EMU', '', 1)

    def _setup_for_multiple_runs(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def _setup_for_coadd_mode(self):
        self.mock_current_runs = mock.PropertyMock(return_value=self.coadd_runs)
        type(self.model._data_context).current_runs = self.mock_current_runs

    def tearDown(self):
        self.model = None
        AnalysisDataService.clear()

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_not_in_coadd_mode(self):
        self._setup_for_multiple_runs()
        self.assertEqual(self.model.number_of_run_strings, len(self.runs))

    def test_that_number_of_run_strings_returns_the_expected_number_of_runs_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.number_of_run_strings, len(self.coadd_runs))

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_not_in_coadd_mode(self):
        self._setup_for_multiple_runs()
        self.assertEqual(self.model.run_number_strings(), ["84447", "84448", "84449"])

    def test_that_run_number_strings_returns_the_expected_run_number_strings_when_in_coadd_mode(self):
        self._setup_for_coadd_mode()
        self.assertEqual(self.model.run_number_strings(), ["84447-84449"])

    def test_that_current_runs_returns_none_when_a_run_string_is_not_selected(self):
        self.assertEqual(self.model.current_runs(), None)

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_non_coadd_mode(self):
        self._setup_for_multiple_runs()

        self.model.set_current_run_string("84448")

        self.assertEqual(self.model.current_runs(), [84448])

    def test_that_current_runs_returns_the_runs_list_corresponding_to_the_currently_selected_string_in_coadd_mode(self):
        self._setup_for_coadd_mode()

        self.model.set_current_run_string("84447-84449")

        self.assertEqual(self.model.current_runs(), [84447, 84448, 84449])

    def test_that_calculate_asymmetry_workspaces_for_will_calculate_the_expected_asymmetry_workspace(self):
        self.context.calculate_all_counts()

        self.model.calculate_asymmetry_workspaces_for([f"{self.run_number}", f"{self.run_number}"], ["fwd", "bwd"])

        self._assert_workspaces_exist(["EMU19489; Group; fwd; Asymmetry; MA", "EMU19489; Group; bwd; Asymmetry; MA"])

    def test_that_calculate_pairs_for_will_calculate_the_expected_pair_asymmetry_workspaces(self):
        self.context.calculate_all_counts()
        self.model.calculate_asymmetry_workspaces_for([f"{self.run_number}", f"{self.run_number}"], ["fwd", "bwd"])

        pair_names = self.model.calculate_pairs_for([f"{self.run_number}"], ["bwd"])

        self.assertEqual(pair_names, ["long"])
        self._assert_workspaces_exist(["EMU19489; Pair Asym; long; MA"])

    def test_that_calculate_diffs_for_will_calculate_the_expected_diff_asymmetry_workspaces(self):
        diff = MuonDiff('group_diff', 'fwd', 'bwd')
        self.context.group_pair_context.add_diff(diff)

        self.context.calculate_all_counts()
        self.model.calculate_asymmetry_workspaces_for([f"{self.run_number}", f"{self.run_number}"], ["fwd", "bwd"])

        self.model.calculate_diffs_for([f"{self.run_number}"], ["bwd"])

        self._assert_workspaces_exist(["EMU19489; Diff; group_diff; Asymmetry; MA"])

    def _assert_workspaces_exist(self, workspace_names: list):
        ads_list = AnalysisDataService.getObjectNames()
        for workspace_name in workspace_names:
            self.assertTrue(workspace_name in ads_list)


if __name__ == '__main__':
    unittest.main()
