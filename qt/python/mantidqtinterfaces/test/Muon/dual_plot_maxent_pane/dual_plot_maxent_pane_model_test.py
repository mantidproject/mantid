# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqtinterfaces.Muon.GUI.Common.ADSHandler.workspace_naming import (
    MAXENT_STR,
    get_maxent_workspace_name,
    get_raw_data_workspace_name,
    RECONSTRUCTED_SPECTRA,
)
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.utilities.algorithm_utils import create_empty_table
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.raw_pane.raw_pane_model import RawPaneModel
from mantidqtinterfaces.Muon.GUI.Common.plot_widget.data_pane.plot_data_pane_model import PlotDataPaneModel
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.dual_plot_maxent_pane_model import (
    DualPlotMaxentPaneModel,
)


def gen_table():
    tab = create_empty_table("table")
    tab.addColumn("str", "Group")
    tab.addColumn("str", "Detectors")
    groups = ["fwd", "bwd"]
    detectors_list = ["0,1,2,3,4", "7,8,9,10"]
    for j, name in enumerate(groups):
        detectors = detectors_list[j]
        tab.addRow([name, detectors])
    return tab


class DuelPlotMaxentPaneModelTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.context = setup_context()

    @classmethod
    def tearDownClass(cls):
        cls.context.ads_observer = None

    def setUp(self):
        self.raw = mock.Mock(spec=RawPaneModel)
        self.group = mock.Mock(spec=PlotDataPaneModel)

        self.model = DualPlotMaxentPaneModel(context=self.context, time_group_model=self.group, raw_model=self.raw)
        test_group = MuonGroup(group_name="fwd", detector_ids=[1, 2, 3, 4, 5])
        self.context.group_pair_context._groups = [test_group]
        self.context.group_pair_context._pairs = []
        self.context.group_pair_context._selected_groups = ["fwd"]
        self.context.group_pair_context._selected_pairs = []
        self.context.data_context.instrument = "MUSR"
        self.count = 0

    def test_clear_data(self):
        self.model.reconstructed_data = {"test": 1}
        self.model.reconstructed_data_name = "unit"
        self.model._selection = "mantid"

        self.model.clear_data()
        self.assertEqual(self.model.reconstructed_data, {})
        self.assertEqual(self.model.reconstructed_data_name, "")
        self.assertEqual(self.model._selection, "")

    def test_create_options(self):
        self.model.set_if_groups(True)
        self.model.get_group_options = mock.Mock(return_value=["fwd", "bwd"])

        self.assertEqual(self.model.create_options(), ["fwd", "bwd"])

    def test_create_options_dets(self):
        self.model.set_if_groups(False)
        self.raw.gen_detector_options = mock.Mock(return_value=["1", "2"])

        self.assertEqual(self.model.create_options(), ["1", "2"])

    @mock.patch(
        "mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.plot_widget.dual_plot_maxent_pane.dual_plot_maxent_pane_model.retrieve_ws"
    )
    def test_set_reconstructed_data(self, mock_retrieve):
        mock_retrieve.return_value = gen_table()
        self.assertEqual(self.model.reconstructed_data_name, "")

        self.model.set_reconstructed_data("data", "table")
        self.assertEqual(self.model.reconstructed_data_name, "data")
        self.assertEqual(self.model.reconstructed_data, {0: "fwd", 1: "bwd"})

    def test_set_reconstructed_data_no_table(self):
        self.assertEqual(self.model.reconstructed_data_name, "")

        self.model.set_reconstructed_data("data", None)
        self.assertEqual(self.model.reconstructed_data_name, "data")
        self.assertEqual(self.model.reconstructed_data, {})

    def test_add_reconstructed_data_groups(self):
        self.model.set_if_groups(True)
        max_val = 2
        self.model.get_first_and_last_group_by_index = mock.Mock(return_value=(0, max_val))
        self.model.reconstructed_data_name = "test"

        ws_list, indices = self.model.add_reconstructed_data([], [])
        # include 0, 1 and 2
        self.assertEqual(ws_list, ["test", "test", "test"])
        self.assertEqual(indices, [0, 1, 2])

    def test_add_reconstructed_data_dets(self):
        self.model.set_if_groups(False)
        max_val = 2
        self.model.get_first_and_last_detector_by_index = mock.Mock(return_value=(0, max_val))
        self.model.reconstructed_data_name = "test"

        ws_list, indices = self.model.add_reconstructed_data([], [])
        # dont include the max_val
        self.assertEqual(ws_list, ["test", "test"])
        self.assertEqual(indices, [0, 1])

    def test_create_workspace_label_freq(self):
        name = get_raw_data_workspace_name("MUSR", "62260", False, 1, "FD")
        name = get_maxent_workspace_name(name, "by groups")
        self.model.set_if_groups(True)
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}

        label = self.model._create_workspace_label(name, 0)
        self.assertEqual(label, "62260 Frequency ;MaxEnt")

    def test_create_workspace_label_group_reconstructed(self):
        name = get_raw_data_workspace_name("MUSR", "62260", False, 1, "FD")
        name = get_maxent_workspace_name(name, "by groups")
        name += RECONSTRUCTED_SPECTRA
        self.model.set_if_groups(True)
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}

        label = self.model._create_workspace_label(name, 0)
        self.assertEqual(label, "62260 fwd ;MaxEnt" + RECONSTRUCTED_SPECTRA)

        label = self.model._create_workspace_label(name, 1)
        self.assertEqual(label, "62260 bwd ;MaxEnt" + RECONSTRUCTED_SPECTRA)

    def test_create_workspace_label_dets_reconstructed(self):
        name = get_raw_data_workspace_name("MUSR", "62260", False, 1, "FD")
        name = get_maxent_workspace_name(name, "All detectors")
        name += RECONSTRUCTED_SPECTRA
        self.model.set_if_groups(False)
        self.raw._create_workspace_label = mock.Mock(return_value="det_label")

        label = self.model._create_workspace_label(name, 3)
        self.raw._create_workspace_label.assert_called_once_with(name, 3)
        self.assertEqual(label, "det_label" + RECONSTRUCTED_SPECTRA)

    def test_create_workspace_label_group(self):
        name = "MUSR62260; Group; fwd; Counts; FD"
        self.model.set_if_groups(True)
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}

        label = self.model._create_workspace_label(name, 0)
        self.assertEqual(label, "62260 fwd")

        label = self.model._create_workspace_label(name, 1)
        self.assertEqual(label, "62260 bwd")

    def test_create_workspace_label_dets(self):
        name = get_raw_data_workspace_name("MUSR", "62260", False, 1, "FD")
        self.model.set_if_groups(False)
        self.raw._create_workspace_label = mock.Mock(return_value="det_label")

        label = self.model._create_workspace_label(name, 3)
        self.raw._create_workspace_label.assert_called_once_with(name, 3)
        self.assertEqual(label, "det_label")

    def test_get_worksapce_list_and_indices_to_plot_groups(self):
        self.model.set_if_groups(True)
        self.model.get_group_list = mock.Mock(return_value=(["fwd", "bwd"]))
        self.group.get_workspace_list_and_indices_to_plot = mock.Mock(return_value=(["unit", "test"], [0, 1]))
        ws, indices = self.model.get_workspace_list_and_indices_to_plot()

        self.group.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Counts", ["fwd", "bwd"])
        self.assertEqual(ws, ["unit", "test"])
        self.assertEqual(indices, [0, 1])

    def test_get_worksapce_list_and_indices_to_plot_dets(self):
        self.model.set_if_groups(False)
        self.model._run = "62260"
        self.model._selection = "5:7"
        self.raw.get_workspace_list_and_indices_to_plot = mock.Mock(return_value=(["unit", "test"], [5, 6]))
        ws, indices = self.model.get_workspace_list_and_indices_to_plot()

        self.raw.get_workspace_list_and_indices_to_plot.assert_called_once_with(True, "Counts", "5:7", "62260", 1)
        self.assertEqual(ws, ["unit", "test"])
        self.assertEqual(indices, [5, 6])

    def test_create_tiled_plot_groups(self):
        self.model.set_if_groups(True)
        self.model.get_group_list = mock.Mock(return_value=["fwd", "bwd"])

        expected = ["Maxent", "fwd", "bwd"]
        self.assertEqual(self.model.create_tiled_keys(""), expected)

    def test_create_tiled_plot_dets(self):
        self.model.set_if_groups(False)
        self.model.get_first_and_last_detector_by_index = mock.Mock(return_value=(3, 6))
        self.raw.create_tiled_keys = mock.Mock(return_value=["3", "4", "5", "6"])

        expected = ["Maxent", "3", "4", "5", "6"]
        self.assertEqual(self.model.create_tiled_keys(""), expected)
        self.raw.create_tiled_keys.assert_called_once_with("", 3 + 1)

    def test_get_workspace_plot_axis_not_tiled(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=False)
        name = "MUSR62260"
        axis_map = {"Maxent": 1, "fwd": 3}

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map), 0)

    def test_get_workspaces_to_plot_axis_maxent(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        name = "MUSR62260" + MAXENT_STR
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 1)

    def test_get_workspaces_to_plot_axis_maxent_group(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        type(self.model)._is_groups = mock.PropertyMock(return_value=True)
        name = "MUSR62260" + MAXENT_STR + RECONSTRUCTED_SPECTRA
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 5)

    def test_get_workspaces_to_plot_axis_maxent_det(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        type(self.model)._is_groups = mock.PropertyMock(return_value=False)
        name = "MUSR62260" + MAXENT_STR + RECONSTRUCTED_SPECTRA
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}
        self.raw.convert_index_to_axis = mock.Mock(return_value=10)

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 11)
        self.raw.convert_index_to_axis.assert_called_once_with(1)

    def test_get_workspaces_to_plot_axis_group(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        type(self.model)._is_groups = mock.PropertyMock(return_value=True)
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name = mock.Mock(return_value=("fwd", 62260))
        name = "MUSR62260"
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 3)

    def test_get_workspaces_to_plot_axis_group_not_found(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        type(self.model)._is_groups = mock.PropertyMock(return_value=True)
        self.context.group_pair_context.get_group_pair_name_and_run_from_workspace_name = mock.Mock(return_value=("top", 62260))
        name = "MUSR62260"
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 0)

    def test_get_workspaces_to_plot_axis_det(self):
        type(self.context.plot_panes_context[self.model.name].settings)._is_tiled = mock.PropertyMock(return_value=True)
        type(self.model)._is_groups = mock.PropertyMock(return_value=False)
        name = "MUSR62260"
        axis_map = {"Maxent": 1, "fwd": 3, "bwd": 5}
        self.raw.convert_index_to_axis = mock.Mock(return_value=10)

        self.assertEqual(self.model._get_workspace_plot_axis(name, axis_map, 1), 11)
        self.raw.convert_index_to_axis.assert_called_once_with(1)

    def test_get_first_and_last_detector_by_index(self):
        self.raw._get_first_and_last_detector_to_plot = mock.Mock(return_value=(2, 4))
        self.model._selection = "1:4"

        self.assertEqual(self.model.get_first_and_last_detector_by_index(), (2, 4))
        self.raw._get_first_and_last_detector_to_plot.assert_called_once_with("1:4")

    def test_get_index_from_group(self):
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}
        self.assertEqual(self.model._get_index_from_group("bwd"), 1)

    def test_get_index_from_group_fail(self):
        self.model.reconstructed_data = {0: "fwd", 1: "bwd"}
        self.assertEqual(self.model._get_index_from_group("top"), 0)

    def test_get_first_and_last_group_by_index_fail(self):
        self.model._selection = ""
        self.assertEqual(self.model.get_first_and_last_group_by_index(), (0, 1))

    def index_from_group(self, group):
        self.count += 1
        return self.count

    def test_get_first_and_last_group_by_index(self):
        self.model._selection = "fwd:bwd"
        self.model._get_index_from_group = mock.Mock(side_effect=self.index_from_group)

        self.assertEqual(self.model.get_first_and_last_group_by_index(), (1, 2))
        self.model._get_index_from_group.assert_any_call("fwd")
        self.model._get_index_from_group.assert_any_call("bwd")
        self.assertEqual(self.model._get_index_from_group.call_count, 2)

    def test_get_group_list_fail(self):
        self.assertEqual(self.model.get_group_list(), [])

    def test_get_group_list(self):
        self.model.reconstructed_data = {0: "fwd", 1: "bwd", 2: "top", 3: "bottom", 4: "left"}
        self.model._selection = "bwd:bottom"
        self.assertEqual(self.model.get_group_list(), ["bwd", "top", "bottom"])

    def test_get_group_options_fail(self):
        self.assertEqual(self.model.get_group_options(), [])

    def test_get_group_options_multiple_selectors(self):
        self.model.reconstructed_data = {0: "fwd", 1: "bwd", 2: "top", 3: "bottom", 4: "left"}
        self.model._time_spec_limit = 2
        self.assertEqual(self.model.get_group_options(), ["fwd:bwd", "top:bottom", "left:left"])

    def test_get_group_options_single_selector(self):
        self.model.reconstructed_data = {0: "fwd", 1: "bwd", 2: "top", 3: "bottom", 4: "left"}
        self.model._time_spec_limit = 10
        self.assertEqual(self.model.get_group_options(), ["fwd:left"])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
