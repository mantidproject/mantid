# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.fitting_tab.ea_fitting_tab_model import EAFittingTabModel
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests
from mantid.api import IFunction


class EAFittingTabModelTest(unittest.TestCase):

    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = EAFittingTabModel(self.context, self.fitting_context)

        group1 = EAGroup("9999; Detector 1", "Detector 1", "9999")
        self.group_context.add_group(group1)
        self.group_context.add_group_to_selected_groups("9999; Detector 1")

        group2 = EAGroup("9999; Detector 2", "Detector 2", "9999")
        self.group_context.add_group(group2)
        self.group_context.add_group_to_selected_groups("9999; Detector 2")

        group3 = EAGroup("9998; Detector 1", "Detector 1", "9999")
        self.group_context.add_group(group3)
        self.group_context.add_group_to_selected_groups("9998; Detector 1")

    def test_get_selected_runs_and_detectors(self):
        runs, detectors = self.model._get_selected_runs_and_detectors()
        self.assertCountEqual(runs, ["9999", "9998"])
        self.assertCountEqual(detectors, ["Detector 1", "Detector 2"])

    def test_get_workspace_names_to_display_from_context_when_simultaneously_fitting(self):
        self.model._check_data_exists = mock.Mock()
        self.context.get_workspace_names_for = mock.Mock(return_value="mock_value")
        self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode = mock.Mock(return_value=["All",
                                                                                             ["Detector 1"]])
        self.fitting_context.fit_to_raw = True
        self.fitting_context.simultaneous_fitting_mode = True
        self.model.get_workspace_names_to_display_from_context()
        self.context.get_workspace_names_for.assert_called_once_with("All", ["Detector 1"], True)
        self.model._check_data_exists.assert_called_once_with("mock_value")

    def test_get_workspace_names_to_display_from_context_when_fitting(self):
        self.model._check_data_exists = mock.Mock()
        self.context.get_workspace_names_for = mock.Mock(return_value="mock_value")
        self.fitting_context.simultaneous_fitting_mode = False
        self.model.get_workspace_names_to_display_from_context()
        self.context.get_workspace_names_for.assert_called_once_with("All", ["9999; Detector 1", "9999; Detector 2",
                                                                             "9998; Detector 1"], False)
        self.model._check_data_exists.assert_called_once_with("mock_value")

    def test_get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode_when_fitting_by_run(self):
        self.model.simultaneous_fit_by = "Run"
        self.model.simultaneous_fit_by_specifier = "9999"
        run, detector = self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode()
        self.assertEqual(run, "9999")
        self.assertEqual(detector, [])

    def test_get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode_when_fitting_by_detector(self):
        self.model.simultaneous_fit_by = "Detector"
        self.model.simultaneous_fit_by_specifier = "Detector 1"
        run, detector = self.model._get_selected_runs_groups_and_pairs_for_simultaneous_fit_mode()
        self.assertEqual(run, "All")
        self.assertEqual(detector, ["Detector 1"])

    def test_get_parameters_for_single_fit(self):
        self.model._get_common_parameters = mock.Mock(return_value={"Minimizer": "1", "EvaluationType": "2"})
        self.model.current_spectrum = "3"
        start_x = 0.0
        end_x = 1000.0
        function = mock.Mock(spec=IFunction)
        dataset_name = "9999; Detector 1"
        params = self.model._get_parameters_for_single_fit(dataset_name, function)
        correct_params = {"Minimizer": "1", "EvaluationType": "2", "Function": function, "WorkspaceIndex": "3",
                          "InputWorkspace": dataset_name, "StartX": start_x, "EndX": end_x}
        self.assertEqual(params, correct_params)

    def test_get_parameters_for_simultaneous_fit(self):
        self.model._get_common_parameters = mock.Mock(return_value={"Minimizer": "1", "EvaluationType": "2"})
        self.model.current_spectrum = "3"
        self.fitting_context._start_xs = [0, 2]
        self.fitting_context._end_xs = [8, 10]
        function = mock.Mock(spec=IFunction)
        dataset_names = ["9999; Detector 1", "9999; Detector 2"]
        params = self.model._get_parameters_for_simultaneous_fit(dataset_names, function)
        correct_params = {"Minimizer": "1", "EvaluationType": "2", "Function": function, "WorkspaceIndex": "3",
                          "InputWorkspace": dataset_names, "StartX": [0, 2], "EndX": [8, 10]}
        self.assertEqual(params, correct_params)

    def test_get_simultaneous_fit_by_specifiers_to_display_from_context_when_detector_selected(self):
        self.model.simultaneous_fit_by = "Detector"
        specifiers = self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        self.assertCountEqual(specifiers, ["Detector 1", "Detector 2"])

    def test_get_simultaneous_fit_by_specifiers_to_display_from_context_when_run_selected(self):
        self.model.simultaneous_fit_by = "Run"
        specifiers = self.model.get_simultaneous_fit_by_specifiers_to_display_from_context()
        self.assertCountEqual(specifiers, ["9999", "9998"])

    def test_get_equivalent_binned_or_unbinned_workspaces(self):
        input_list = ["mock_workspace"]
        self.fitting_context.fit_to_raw = True
        self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents = mock.Mock()
        self.fitting_context.dataset_names = input_list

        self.model._get_equivalent_binned_or_unbinned_workspaces()

        self.context.get_list_of_binned_or_unbinned_workspaces_from_equivalents.assert_called_once_with(input_list,
                                                                                                        True)


if __name__ == '__main__':
    unittest.main()
