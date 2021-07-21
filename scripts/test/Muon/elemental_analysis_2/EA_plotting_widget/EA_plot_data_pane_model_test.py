# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.ElementalAnalysis2.plotting_widget.EA_plotting_pane.EA_plot_data_pane_model import EAPlotDataPaneModel
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_ea_tests


@start_qapplication
class EAPlotDataPaneModelTest(unittest.TestCase):

    def setUp(self):
        setup_context_for_ea_tests(self)
        self.model = EAPlotDataPaneModel(context=self.context)
        self.name = "9999; Detector 1"
        test_group = EAGroup(group_name=self.name, detector="Detector 1", run_number="9999")
        self.context.group_context._groups = [test_group]
        self.context.group_context._selected_groups = [self.name]

    def test_get_workspace_to_plot_counts(self):
        self.context.group_context[self.name].get_counts_workspace_for_run = mock.MagicMock()

        self.model.get_count_workspaces_to_plot(self.name, True)

        self.context.group_context[self.name].get_counts_workspace_for_run.assert_called_once_with(False)

    def test_get_workspace_to_plot_counts_rebin(self):
        group = self.context.group_context[self.name]
        group.get_counts_workspace_for_run = mock.MagicMock()
        group.get_rebined_or_unbinned_version_of_workspace_if_it_exists = mock.MagicMock()

        self.model.get_count_workspaces_to_plot(self.name, False)

        group.get_counts_workspace_for_run.assert_not_called()
        group.get_rebined_or_unbinned_version_of_workspace_if_it_exists.assert_called_once()

    def test_get_workspace_and_indices_for_group_or_pair_returns_correctly(self):
        self.model.get_count_workspaces_to_plot = mock.Mock(return_value=[self.name])
        expected_workspaces = [self.name]
        expected_indices = [2]

        workspaces, indices = self.model.get_workspace_and_indices_for_group(self.name, True, "Total")

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_workspaces_to_plot(self):
        self.model.get_count_workspaces_to_plot = mock.Mock(return_value="test")
        self.context.group_context._selected_groups = ["9999; Detector 2", "9999; Detector 3", "9999; Detector 4"]
        self.model.get_workspaces_to_plot(True)
        self.assertEqual(self.model.get_count_workspaces_to_plot.call_count, 3)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 2", True)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 3", True)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 4", True)

    def test_get_workspace_list_and_indices_to_plot(self):
        self.model.get_workspaces_to_plot = mock.Mock(return_value=["test"])
        self.model._generate_run_indices = mock.Mock(return_value=[0])

        self.model.get_workspace_list_and_indices_to_plot(True, "Delayed")
        self.model.get_workspaces_to_plot.assert_called_once_with(True)
        self.model._generate_run_indices.assert_called_once_with(["test"], "Delayed")

    def test_get_workspaces_to_remove(self):
        self.model.get_count_workspaces_to_plot = mock.Mock(return_value="test")
        names = ["9999; Detector 2", "9999; Detector 3", "9999; Detector 4"]

        self.model.get_workspaces_to_remove(names)
        self.assertEqual(self.model.get_count_workspaces_to_plot.call_count, 6)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 2", True)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 3", True)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 4", True)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 2", False)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 3", False)
        self.model.get_count_workspaces_to_plot.assert_any_call("9999; Detector 4", False)

    def test_create_tiled_keys_returns_correctly_for_tiled_by_run(self):
        self.context.group_context.add_group(EAGroup(group_name="9997; Detector 2", detector="Detector 2",
                                                     run_number="9997"))
        self.context.group_context.add_group(EAGroup(group_name="9998; Detector 3", detector="Detector 3",
                                                     run_number="9998"))
        self.context.group_context.add_group(EAGroup(group_name="9999; Detector 4", detector="Detector 4",
                                                     run_number="9999"))
        self.context.group_context._selected_groups = ["9997; Detector 2", "9999; Detector 4"]
        runs = [[9997, 9998, 9999]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys("Run")

        self.assertEqual(keys, ['9997', '9999'])

    def test_create_tiled_keys_returns_correctly_for_tiled_by_group(self):
        self.context.group_context.add_group(EAGroup(group_name="9998; Detector 1", detector="Detector 1",
                                                     run_number="9999"))
        self.context.group_context.add_group(EAGroup(group_name="9999; Detector 2", detector="Detector 2",
                                                     run_number="9998"))
        self.context.group_context._selected_groups = ["9999; Detector 1", "9998; Detector 1", "9999; Detector 2"]
        runs = [[9999], [9998]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys("Detector")

        self.assertEqual(keys, ["Detector 1", "Detector 2"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
