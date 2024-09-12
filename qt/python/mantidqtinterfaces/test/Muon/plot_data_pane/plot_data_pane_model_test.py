# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from mantidqt.utils.qt.testing import start_qapplication
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context


@start_qapplication
class PlotDataPaneModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotDataPaneModel(context=self.context)
        test_group = MuonGroup(group_name="fwd", detector_ids=[1, 2, 3, 4, 5])
        self.context.group_pair_context._groups = [test_group]
        self.context.group_pair_context._pairs = []
        self.context.group_pair_context._selected_groups = ["fwd"]
        self.context.group_pair_context._selected_pairs = []

    def test_get_workspace_to_plot_time(self):
        runs = [[62260]]
        self.context.data_context.current_runs = runs
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names = mock.MagicMock()

        self.model.get_time_workspaces_to_plot("fwd", True, "Asymmetry")

        self.assertEqual(self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.call_count, 1)
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.assert_called_once_with(runs)

    def test_get_workspace_to_plot_time_rebin(self):
        runs = [[62260]]
        self.context.data_context.current_runs = runs
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names = mock.MagicMock()
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names_rebinned = mock.MagicMock()

        self.model.get_time_workspaces_to_plot("fwd", False, "Asymmetry")

        self.assertEqual(self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.call_count, 0)
        self.assertEqual(self.context.group_pair_context["fwd"].get_asymmetry_workspace_names_rebinned.call_count, 1)
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names_rebinned.assert_called_once_with(runs)

    def test_get_workspace_to_plot_time_counts(self):
        runs = [[62260]]
        self.context.data_context.current_runs = runs
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names = mock.MagicMock()

        ws_list = self.model.get_time_workspaces_to_plot("fwd", True, "Counts")

        self.assertEqual([], ws_list)

    def test_get_workspace_and_indices_for_group_or_pair_returns_correctly(self):
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;bwd"])
        expected_workspaces = ["62260;bwd"]
        expected_indices = [0]

        workspaces, indices = self.model.get_workspace_and_indices_for_group_or_pair("bwd", True, "Asymmetry")

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_workspaces_to_plot(self):
        selected = ["fwd", "bwd", "top"]
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value="test")
        self.context.group_pair_context.add_group(MuonGroup(group_name="bwd", detector_ids=[2]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="top", detector_ids=[3]))
        self.context.group_pair_context._selected_groups = selected
        self.model.get_workspaces_to_plot(True, "Counts", selected)
        self.assertEqual(self.model.get_time_workspaces_to_plot.call_count, 3)
        self.model.get_time_workspaces_to_plot.assert_any_call("fwd", True, "Counts")
        self.model.get_time_workspaces_to_plot.assert_any_call("bwd", True, "Counts")
        self.model.get_time_workspaces_to_plot.assert_any_call("top", True, "Counts")

    def test_get_workspace_list_and_indices_to_plot(self):
        self.model.get_workspaces_to_plot = mock.Mock(return_value=["test"])
        self.model._generate_run_indices = mock.Mock(return_value=[0])

        self.model.get_workspace_list_and_indices_to_plot(True, "Counts")
        self.model.get_workspaces_to_plot.assert_called_once_with(True, "Counts", ["fwd"])
        self.model._generate_run_indices.assert_called_once_with(["test"])

    def test_get_workspaces_to_remove(self):
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value="test")
        names = ["fwd", "bwd", "top"]

        self.model.get_workspaces_to_remove(names, True, "Counts")
        self.assertEqual(self.model.get_time_workspaces_to_plot.call_count, 3)
        self.model.get_time_workspaces_to_plot.assert_any_call("fwd", True, "Counts")
        self.model.get_time_workspaces_to_plot.assert_any_call("bwd", True, "Counts")
        self.model.get_time_workspaces_to_plot.assert_any_call("top", True, "Counts")

    def test_create_tiled_keys_returns_correctly_for_tiled_by_run(self):
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260], [62261]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys("Run")

        self.assertEqual(keys, ["62260", "62261"])

    def test_create_tiled_keys_returns_correctly_for_summed_runs_tiled_by_run(self):
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260, 62261]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys("Run")

        self.assertEqual(keys, ["62260-62261"])

    def test_create_tiled_keys_returns_correctly_for_tiled_by_group(self):
        self.context.group_pair_context.add_group(MuonGroup(group_name="bwd", detector_ids=[2]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="top", detector_ids=[3]))
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260], [62261]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys("Group/Pair")

        self.assertEqual(keys, ["fwd", "bwd", "top"])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
