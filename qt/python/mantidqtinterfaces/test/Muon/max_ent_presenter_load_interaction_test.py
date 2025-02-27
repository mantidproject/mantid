# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FileFinder
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy import QtCore
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_view
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context
from mantidqtinterfaces.Muon.GUI.Common.utilities import load_utils
from mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt import maxent_presenter
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.general_test_helpers import create_workspace_wrapper_stub_object
from mantid.simpleapi import CreateWorkspace


def retrieve_combobox_info(combo_box):
    output_list = []
    for i in range(combo_box.count()):
        output_list.append(str(combo_box.itemText(i)))

    return output_list


class mock_ws(object):
    def __init__(self, name):
        self._name = name

    def name(self):
        return self._name


class mock_wrapper(object):
    def __init__(self, name):
        self.workspace = mock_ws(name)


@start_qapplication
class MaxEntPresenterTest(unittest.TestCase):
    def setUp(self):
        self.context = setup_context(True)

        self.context.data_context.instrument = "MUSR"
        self.context.data_context.num_periods = mock.Mock(return_value=1)

        self.context.gui_context.update({"RebinType": "None"})

        self.view = maxent_view.MaxEntView()

        self.presenter = maxent_presenter.MaxEntPresenter(self.view, self.context)

        file_path = FileFinder.findRuns("MUSR00022725.nxs")[0]
        ws, run, filename, _ = load_utils.load_workspace_from_filename(file_path)
        self.context.data_context._loaded_data.remove_data(run=run)
        self.context.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument="MUSR")
        self.context.data_context.current_runs = [[22725]]

        self.context.update_current_data()
        test_pair = MuonPair("test_pair", "top", "bottom", alpha=0.75)
        self.context.group_pair_context.add_pair(pair=test_pair)
        self._count = 0
        self.view.warning_popup = mock.MagicMock()

    def test_load_single_period(self):
        self.presenter.getWorkspaceNames()
        self.view.add_periods = mock.Mock()
        self.presenter._load_periods()
        self.view.add_periods.assert_called_once_with(["1"])

    def test_load_multiple_periods(self):
        self.context.data_context.num_periods.return_value = 4
        self.presenter.getWorkspaceNames()
        self.view.add_periods = mock.Mock()
        self.presenter._load_periods()
        self.view.add_periods.assert_called_once_with(["1", "2", "3", "4"])

    def test_load_periods_no_run(self):
        self.view.add_periods = mock.Mock()
        self.presenter._load_periods()
        self.view.add_periods.assert_called_once_with([])

    def test_get_Workspace_names(self):
        self.view.addRuns = mock.Mock()
        self.presenter._load_periods = mock.Mock()
        self.presenter.getWorkspaceNames()

        self.presenter._load_periods.assert_called_once_with()
        self.view.addRuns.assert_called_once_with(["22725"])

    def setup_handle_finished(self):
        self.presenter._maxent_output_workspace_name = "output"
        self.presenter.activate = mock.Mock()
        self.presenter.calculation_finished_notifier.notify_subscribers = mock.Mock()
        self.presenter.new_reconstructed_data.notify_subscribers = mock.Mock()
        self.context.frequency_context.add_group_phase_table = mock.Mock()
        self.context.phase_context.add_phase_table = mock.Mock()
        self.presenter.new_phase_table.notify_subscribers = mock.Mock()
        self.presenter.update_phase_table_options = mock.Mock()

    def wrapper_side_effect(self, _):
        self._count += 1
        if self._count == 1:
            return mock_wrapper("workspace")
        if self._count == 2:
            return mock_wrapper("table")
        return "not expected"

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.MuonWorkspaceWrapper")
    def test_handle_finished_reconstructed_with_groups(self, wrapper_mock):
        wrapper_mock.side_effect = self.wrapper_side_effect
        self.setup_handle_finished()
        self.presenter._optional_output_names = {"ReconstructedSpectra": "calculated_data", "GroupingTable": "table"}
        type(self.view).output_reconstructed_spectra = mock.PropertyMock(return_value=True)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)

        self.presenter.handleFinished()
        self.assertEqual(self.presenter.activate.call_count, 1)
        self.assertEqual(self.presenter.new_reconstructed_data.notify_subscribers.call_count, 1)
        self.presenter.new_reconstructed_data.notify_subscribers.assert_called_once_with({"ws": "workspace", "table": "table"})

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.MuonWorkspaceWrapper")
    def test_handle_finished_reconstructed_with_dets(self, wrapper_mock):
        wrapper_mock.side_effect = self.wrapper_side_effect
        self.setup_handle_finished()
        self.presenter._optional_output_names = {"ReconstructedSpectra": "calculated_data", "GroupingTable": "table"}
        type(self.view).output_reconstructed_spectra = mock.PropertyMock(return_value=True)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=False)

        self.presenter.handleFinished()
        self.assertEqual(self.presenter.activate.call_count, 1)
        self.assertEqual(self.presenter.new_reconstructed_data.notify_subscribers.call_count, 1)
        self.presenter.new_reconstructed_data.notify_subscribers.assert_called_once_with({"ws": "workspace", "table": None})

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.MuonWorkspaceWrapper")
    def test_handle_finished(self, wrapper_mock):
        wrapper_mock.return_value = "table_ws"
        self.setup_handle_finished()
        self.presenter._optional_output_names = {"OutputPhaseTable": "table"}
        type(self.view).output_phase_table = mock.PropertyMock(return_value=True)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)

        self.presenter.handleFinished()
        self.assertEqual(self.presenter.activate.call_count, 1)
        self.context.frequency_context.add_group_phase_table.assert_called_once_with("table_ws", 2)
        self.assertEqual(self.context.phase_context.add_phase_table.call_count, 0)
        self.assertEqual(self.presenter.new_phase_table.notify_subscribers.call_count, 0)
        self.assertEqual(self.presenter.update_phase_table_options.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.MuonWorkspaceWrapper")
    def test_handle_finished_not_groups(self, wrapper_mock):
        wrapper_mock.return_value = "table_ws"
        self.setup_handle_finished()
        self.presenter._optional_output_names = {"OutputPhaseTable": "table"}
        type(self.view).output_phase_table = mock.PropertyMock(return_value=True)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=False)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)

        self.presenter.handleFinished()
        self.assertEqual(self.presenter.activate.call_count, 1)
        self.assertEqual(self.context.frequency_context.add_group_phase_table.call_count, 0)
        self.context.phase_context.add_phase_table.assert_called_once_with("table_ws")
        self.assertEqual(self.presenter.new_phase_table.notify_subscribers.call_count, 1)
        self.assertEqual(self.presenter.update_phase_table_options.call_count, 1)

    def test_handle_finished_no_table(self):
        self.setup_handle_finished()
        self.presenter._optional_output_names = {}
        type(self.view).output_phase_table = mock.PropertyMock(return_value=True)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=False)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)

        self.presenter.handleFinished()
        self.assertEqual(self.presenter.activate.call_count, 1)
        self.assertEqual(self.context.frequency_context.add_group_phase_table.call_count, 0)
        self.assertEqual(self.context.phase_context.add_phase_table.call_count, 0)
        self.assertEqual(self.presenter.new_phase_table.notify_subscribers.call_count, 0)
        self.assertEqual(self.presenter.update_phase_table_options.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.run_MuonMaxent")
    def test_calculate_maxent(self, mock_maxent):
        params = {"InputWorkspace": "test"}
        self.presenter.get_parameters_for_maxent_calculation = mock.Mock(return_value=params)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)
        self.presenter.add_maxent_workspace_to_ADS = mock.Mock()
        alg = mock.Mock()

        self.presenter.calculate_maxent(alg)
        self.assertEqual(self.presenter.add_maxent_workspace_to_ADS.call_count, 1)
        self.assertEqual(mock_maxent.call_count, 1)

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.run_MuonMaxent")
    def test_calculate_maxent_fails(self, mock_maxent):
        params = {"InputWorkspace": "test"}
        self.presenter.get_parameters_for_maxent_calculation = mock.Mock(return_value=params)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=0)
        self.presenter.add_maxent_workspace_to_ADS = mock.Mock()
        alg = mock.Mock()

        self.assertRaises(ValueError, self.presenter.calculate_maxent, alg)
        self.assertEqual(self.presenter.add_maxent_workspace_to_ADS.call_count, 0)
        self.assertEqual(mock_maxent.call_count, 0)

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.run_MuonMaxent")
    def test_calculate_maxent_ignores_keyboardinterrupt(self, mock_maxent):
        mock_maxent.side_effect = KeyboardInterrupt()
        params = {"InputWorkspace": "test"}
        self.presenter.get_parameters_for_maxent_calculation = mock.Mock(return_value=params)
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)
        self.presenter.add_maxent_workspace_to_ADS = mock.Mock()
        alg = mock.Mock()

        self.presenter.calculate_maxent(alg)
        self.assertEqual(self.presenter.add_maxent_workspace_to_ADS.call_count, 0)
        self.assertEqual(mock_maxent.call_count, 1)

    def test_create_group_table(self):
        group1 = MuonGroup("fwd", [1, 2, 3])
        type(self.presenter).get_selected_groups = mock.PropertyMock(return_value=[group1])

        tab = self.presenter._create_group_table()
        self.assertEqual(tab.column(0)[0], "fwd")
        self.assertEqual(tab.column(1)[0], "1,2,3")
        self.assertEqual(len(tab.column(0)), 1)
        self.assertEqual(tab.getColumnNames(), ["Group", "Detectors"])

    def test_get_N_points_comboboxes_appropriately(self):
        self.presenter.getWorkspaceNames()

        self.assertEqual(
            retrieve_combobox_info(self.view.N_points),
            ["2048", "4096", "8192", "16384", "32768", "65536", "131072", "262144", "524288", "1048576"],
        )

    def test_get_parameters_for_maxent_calculations(self):
        self.presenter.getWorkspaceNames()
        type(self.presenter).use_groups = mock.PropertyMock(return_value=False)
        self.context.corrections_context.current_dead_time_table_name_for_run = mock.MagicMock(return_value="deadtime_table_name")
        self.context.first_good_data = mock.MagicMock(return_value=0.11)
        self.context.last_good_data = mock.MagicMock(return_value=13.25)
        self.context.phase_context.phase_tables = [
            create_workspace_wrapper_stub_object(x) for x in ["MUSR22222_phase_table", "MUSR33333_phase_table", "EMU22222_phase_table"]
        ]
        self.presenter.update_phase_table_options()
        table = mock.Mock()
        self.presenter._create_group_table = mock.Mock(return_value=table)

        parameters = self.presenter.get_parameters_for_maxent_calculation()

        self.assertEqual(
            parameters,
            {
                "DefaultLevel": 0.1,
                "DoublePulse": False,
                "Factor": 1.04,
                "FirstGoodTime": 0.11,
                "FitDeadTime": True,
                "InnerIterations": 10,
                "InputDeadTimeTable": "deadtime_table_name",
                "InputWorkspace": "MUSR22725_raw_data FD",
                "LastGoodTime": 13.25,
                "MaxField": 1000.0,
                "Npts": 2048,
                "OuterIterations": 10,
            },
        )

    def test_get_parameters_for_maxent_calculations_group(self):
        self.presenter.getWorkspaceNames()
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        self.context.corrections_context.current_dead_time_table_name_for_run = mock.MagicMock(return_value="deadtime_table_name")
        self.context.first_good_data = mock.MagicMock(return_value=0.11)
        self.context.last_good_data = mock.MagicMock(return_value=13.25)
        self.context.phase_context.phase_tables = [
            create_workspace_wrapper_stub_object(x) for x in ["MUSR22222_phase_table", "MUSR33333_phase_table", "EMU22222_phase_table"]
        ]
        self.presenter.update_phase_table_options()
        table = mock.Mock()
        self.presenter._create_group_table = mock.Mock(return_value=table)

        parameters = self.presenter.get_parameters_for_maxent_calculation()

        self.assertEqual(
            parameters,
            {
                "DefaultLevel": 0.1,
                "DoublePulse": False,
                "Factor": 1.04,
                "FirstGoodTime": 0.11,
                "FitDeadTime": True,
                "GroupTable": table,
                "InnerIterations": 10,
                "InputDeadTimeTable": "deadtime_table_name",
                "InputWorkspace": "MUSR22725_raw_data FD",
                "LastGoodTime": 13.25,
                "MaxField": 1000.0,
                "Npts": 2048,
                "OuterIterations": 10,
            },
        )

    def test_update_phase_table_options_group(self):
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)
        self.view.update_phase_table_combo = mock.Mock()
        self.context.frequency_context.get_group_phase_tables = mock.Mock(return_value=["unit", "test"])
        self.context.phase_context.get_phase_table_list = mock.Mock(return_value=["phase", "table"])
        self.presenter.update_phase_table_options()
        self.view.update_phase_table_combo.assert_called_once_with(["None", "unit", "test"])

    def test_update_phase_table_options(self):
        type(self.presenter).use_groups = mock.PropertyMock(return_value=False)
        type(self.presenter).get_num_groups = mock.PropertyMock(return_value=2)
        self.view.update_phase_table_combo = mock.Mock()
        self.context.frequency_context.get_group_phase_tables = mock.Mock(return_value=["unit", "test"])
        self.context.phase_context.get_phase_table_list = mock.Mock(return_value=["phase", "table"])
        self.presenter.update_phase_table_options()
        self.view.update_phase_table_combo.assert_called_once_with(["Construct", "phase", "table"])

    def test_update_phase_table_options_adds_correct_options_to_view_item(self):
        phase_table_names = ["MUSR22222_phase_table", "MUSR33333_phase_table", "EMU22222_phase_table"]
        self.context.phase_context.phase_tables = [create_workspace_wrapper_stub_object(x) for x in phase_table_names]
        type(self.presenter).use_groups = mock.PropertyMock(return_value=True)

        self.presenter.update_phase_table_options()

        self.assertEqual(retrieve_combobox_info(self.view.phase_table_combo), ["None"])

    @mock.patch("mantidqtinterfaces.Muon.GUI.FrequencyDomainAnalysis.MaxEnt.maxent_presenter.MuonWorkspaceWrapper")
    def test_add_maxent_workspace_to_ADS(self, workspace_wrapper_mock):
        self.presenter.getWorkspaceNames()
        self.context.dead_time_table = mock.MagicMock(return_value="deadtime_table_name")
        self.context.first_good_data = mock.MagicMock(return_value=0.11)
        self.context.last_good_data = mock.MagicMock(return_value=13.25)
        self.context.phase_context.phase_tables = [
            create_workspace_wrapper_stub_object(x) for x in ["MUSR22222_phase_table", "MUSR33333_phase_table", "EMU22222_phase_table"]
        ]
        self.presenter.update_phase_table_options()
        CreateWorkspace([0], [0], OutputWorkspace="maxent_workspace")

        self.presenter.add_maxent_workspace_to_ADS("MUSR22725_MaxEnt", "maxent_workspace", mock.MagicMock())

        workspace_wrapper_mock.assert_any_call("MUSR22725 MaxEnt FD/MUSR22725_MaxEnt; by Groups; MaxEnt")
        workspace_wrapper_mock.assert_any_call("GroupingTable")
        self.assertEqual(workspace_wrapper_mock.call_count, 2)

    def test_get_output_options_defaults_returns_correctly(self):
        self.presenter.getWorkspaceNames()

        output_options = self.presenter.get_maxent_output_options()

        self.assertEqual(
            output_options,
            {
                "GroupingTable": True,
                "OutputDeadTimeTable": False,
                "PhaseConvergenceTable": False,
                "OutputPhaseTable": False,
                "ReconstructedSpectra": False,
            },
        )

    def test_get_output_options_returns_correctly(self):
        self.presenter.getWorkspaceNames()
        self.view.output_dead_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_phase_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_phase_evo_box.setCheckState(QtCore.Qt.Checked)
        self.view.output_data_box.setCheckState(QtCore.Qt.Checked)

        output_options = self.presenter.get_maxent_output_options()

        self.assertEqual(
            output_options,
            {
                "GroupingTable": True,
                "OutputDeadTimeTable": True,
                "PhaseConvergenceTable": True,
                "OutputPhaseTable": True,
                "ReconstructedSpectra": True,
            },
        )


if __name__ == "__main__":
    unittest.main()
