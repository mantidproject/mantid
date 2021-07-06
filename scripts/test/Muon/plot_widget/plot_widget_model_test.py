# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import WorkspaceFactory
from Muon.GUI.Common.ADSHandler.workspace_naming import TF_ASYMMETRY_PREFIX
from Muon.GUI.Common.contexts.fitting_contexts.fitting_context import FitInformation
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.plot_widget.plot_widget_model import PlotWidgetModel, TILED_BY_GROUP_TYPE, TILED_BY_RUN_TYPE
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.Common.test_helpers.context_setup import setup_context
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper


@start_qapplication
class PlotWidgetModelTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.model = PlotWidgetModel(context=self.context)
        test_group = MuonGroup(group_name="fwd", detector_ids=[1, 2, 3, 4, 5])
        self.context.group_pair_context._groups = [test_group]
        self.context.group_pair_context._pairs = []
        self.context.group_pair_context._selected_groups = ["fwd"]
        self.context.group_pair_context._selected_pairs = []

    def test_get_workspace_to_plot_calls_correctly_if_frequency_domain(self):
        self.model.get_freq_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])

        self.model.get_workspaces_to_plot(True, "Frequency Re")
        self.assertEquals(self.model.get_freq_workspaces_to_plot.call_count, 1)
        self.assertEquals(self.model.get_time_workspaces_to_plot.call_count, 0)

    def test_get_workspace_to_plot_calls_correctly_if_time_domain(self):
        self.model.get_freq_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])

        self.model.get_workspaces_to_plot(True, "Asymmetry")

        self.assertEquals(self.model.get_freq_workspaces_to_plot.call_count, 0)
        self.assertEquals(self.model.get_time_workspaces_to_plot.call_count, 1)

    def test_get_workspace_to_plot_time(self):
        runs = [[62260]]
        self.context.data_context.current_runs = runs
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names = mock.MagicMock()

        self.model.get_time_workspaces_to_plot("fwd", True, "Asymmetry")

        self.assertEquals(self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.call_count, 1)
        self.context.group_pair_context["fwd"].get_asymmetry_workspace_names.assert_called_once_with(runs)

    def test_get_workspaces_to_plot_freq(self):
        self.context.data_context.current_runs = [[62260]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.model.get_freq_workspaces_to_plot("fwd", "Frequency Re")

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260", "fwd", True,
                                                                                             "Re")

    def test_get_multiple_workspaces_to_plot_freq(self):
        self.context.data_context.current_runs = [[62260], [62261]]
        self.context.get_names_of_frequency_domain_workspaces_to_fit = mock.Mock()
        self.model.get_freq_workspaces_to_plot("fwd", "Frequency Re")

        self.assertEquals(self.context.get_names_of_frequency_domain_workspaces_to_fit.call_count, 1)
        self.context.get_names_of_frequency_domain_workspaces_to_fit.assert_called_once_with("62260, 62261", "fwd",
                                                                                             True, "Re")

    def test_get_fit_workspaces_to_plot_returns_correctly(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()
        fit = FitInformation(mock.MagicMock(), 'GaussOsc',
                             [StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA', workspace)],
                             StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA; Fitted', table_workspace),
                             mock.MagicMock())
        expected_workspaces = ['MUSR62260; Group; bottom; Asymmetry; MA'] * 2
        expected_indices = [1, 2]

        workspaces, indices = self.model.get_fit_workspace_and_indices(fit)

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_fit_workspaces_to_plot_returns_correctly_for_tf_fit(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()
        fit = FitInformation(mock.MagicMock(), 'GaussOsc',
                             [StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA;' + TF_ASYMMETRY_PREFIX,
                                                     workspace)],
                             StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA; Fitted' + TF_ASYMMETRY_PREFIX,
                                                    table_workspace), mock.MagicMock(),
                             tf_asymmetry_fit=True)
        expected_workspaces = ['MUSR62260; Group; bottom; Asymmetry; MA;' + TF_ASYMMETRY_PREFIX] * 2
        expected_indices = [3, 2]

        workspaces, indices = self.model.get_fit_workspace_and_indices(fit)

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_fit_workspaces_to_plot_returns_correctly_when_plot_diff_is_False(self):
        workspace = WorkspaceFactory.create("Workspace2D", NVectors=3, YLength=5, XLength=5)
        table_workspace = WorkspaceFactory.createTable()
        fit = FitInformation(mock.MagicMock(), 'GaussOsc',
                             [StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA', workspace)],
                             StaticWorkspaceWrapper('MUSR62260; Group; bottom; Asymmetry; MA; Fitted', table_workspace),
                             mock.MagicMock())
        expected_workspaces = ['MUSR62260; Group; bottom; Asymmetry; MA']
        expected_indices = [1]

        workspaces, indices = self.model.get_fit_workspace_and_indices(fit, False)

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_workspace_list_and_indices_to_plot_returns_correctly(self):
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;fwd"])
        expected_workspaces = ['62260;fwd']
        expected_indices = [0]

        workspaces, indices = self.model.get_workspace_list_and_indices_to_plot(True, "Asymmetry")

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_get_workspace_and_indices_for_group_or_pair_returns_correctly(self):
        self.model.get_time_workspaces_to_plot = mock.Mock(return_value=["62260;bwd"])
        expected_workspaces = ['62260;bwd']
        expected_indices = [0]

        workspaces, indices = self.model.get_workspace_and_indices_for_group_or_pair("bwd", True, "Asymmetry")

        self.assertEqual(workspaces, expected_workspaces)
        self.assertEqual(expected_indices, indices)

    def test_create_tiled_keys_returns_correctly_for_tiled_by_group(self):
        self.context.group_pair_context.add_group(MuonGroup(group_name="bwd", detector_ids=[6, 7, 8, 9, 10]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="bottom", detector_ids=[11, 12, 13, 14, 15]))
        self.context.group_pair_context.add_group(MuonGroup(group_name="top", detector_ids=[16, 17, 18, 19, 20]))
        self.context.group_pair_context._selected_groups = ["bwd", "fwd", "top"]

        keys = self.model.create_tiled_keys(TILED_BY_GROUP_TYPE)

        self.assertEqual(keys, ["fwd", "bwd", "top"])

    def test_create_tiled_keys_returns_correctly_for_tiled_by_run(self):
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260], [62261]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys(TILED_BY_RUN_TYPE)

        self.assertEqual(keys, ['62260', '62261'])

    def test_create_tiled_keys_returns_correctly_for_summed_runs_tiled_by_run(self):
        self.context.group_pair_context._selected_groups = ["fwd", "bwd", "top"]
        runs = [[62260, 62261]]
        self.context.data_context.current_runs = runs
        keys = self.model.create_tiled_keys(TILED_BY_RUN_TYPE)

        self.assertEqual(keys, ['62260-62261'])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
