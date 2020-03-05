# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.plotting_widget.workspace_finder import WorkspaceFinder
from Muon.GUI.Common.muon_group import MuonGroup


@start_qapplication
class WorkspaceFinderTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)
        self.workspace_finder = WorkspaceFinder(self.context)

        test_group = MuonGroup(group_name="fwd", detector_ids=[1, 2, 3, 4, 5])
        self.context.group_pair_context._groups = [test_group]
        self.context.group_pair_context._pairs = []
        self.context.group_pair_context._selected_groups = ["fwd"]
        self.context.group_pair_context._selected_pairs = []

    def test_get_workspace_to_plot_calls_correctly_if_frequency_domain(self):
        self.workspace_finder.get_freq_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])
        self.workspace_finder.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])

        self.workspace_finder.get_workspaces_to_plot(True, "Frequency Re")
        self.assertEquals(self.workspace_finder.get_freq_workspaces_to_plot.call_count, 1)
        self.assertEquals(self.workspace_finder.get_time_workspaces_to_plot.call_count, 0)

    def test_get_workspace_to_plot_calls_correctly_if_time_domain(self):
        self.workspace_finder.get_freq_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])
        self.workspace_finder.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])

        self.workspace_finder.get_workspaces_to_plot(True, "Asymmetry")

        self.assertEquals(self.workspace_finder.get_freq_workspaces_to_plot.call_count, 0)
        self.assertEquals(self.workspace_finder.get_time_workspaces_to_plot.call_count, 1)

    def test_get_workspace_to_plot_time(self):
        runs = [[62260]]
        self.context.data_context.current_runs = runs
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names = mock.MagicMock()

        self.workspace_finder.get_time_workspaces_to_plot("fwd", True, "Asymmetry")

        self.assertEquals(self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.call_count, 1)
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.assert_called_once_with(runs)

    def test_get_workspaces_to_plot_freq(self):
        self.context.data_context.current_runs = [[62260]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.workspace_finder.get_freq_workspaces_to_plot("fwd", "Frequency Re")

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260", "fwd", True,
                                                                                             "Re")

    def test_get_2_workspaces_to_plot_freq(self):
        self.context.data_context.current_runs = [[62260], [62261]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.workspace_finder.get_freq_workspaces_to_plot("fwd", "Frequency Re")

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260, 62261", "fwd",
                                                                                             True, "Re")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
