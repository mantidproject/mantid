import unittest
import unittest.mock as mock
from mantid.simpleapi import CreateEmptyTableWorkspace, DeleteWorkspace
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import EAAutoTabModel
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from mantid.api import mtd


class EAAutoTabModelTest(unittest.TestCase):

    def setUp(self):
        workspaces = ["9999; Detector 1", "9999; Detector 2", "9999; Detector 3", "9999; Detector 4"]
        self.group_context = EAGroupContext()
        for group_name in workspaces:
            self.group_context.add_group(EAGroup(group_name, group_name.split(";")[1].strip(),
                                                 group_name.split(";")[0].strip()))

        self.context = ElementalAnalysisContext(None, self.group_context)
        self.model = EAAutoTabModel(self.context)

    @staticmethod
    def delete_if_present(workspace):
        if workspace in mtd:
            DeleteWorkspace(workspace)

    def test_update_match_table_when_table_has_more_than_3_rows(self):
        correct_table_entries = ["9999", "Detector 1", "Ag , Au , Fe"]
        # Create source tables
        likelyhood_table = CreateEmptyTableWorkspace(OutputWorkspace="likelyhood")
        likelyhood_table.addColumn("str", "Element")
        likelyhood_table.addColumn("int", "Likelihood")
        likelyhood_table.addRow(["Ag", 20])
        likelyhood_table.addRow(["Au", 18])
        likelyhood_table.addRow(["Fe", 14])
        likelyhood_table.addRow(["Cu", 10])
        likelyhood_table.addRow(["Al", 8])
        likelyhood_table.addRow(["Si", 6])
        likelyhood_table.addRow(["C", 4])
        likelyhood_table.addRow(["O", 3])

        # Run function
        self.model.update_match_table("likelyhood", "9999; Detector 1")

        # Assert statements
        self.assertEqual(self.model.table_entries.get(), correct_table_entries)

        # Delete tables from ADS
        self.delete_if_present("likelyhood")

    def test_update_match_table_when_table_has_less_than_3_rows(self):
        correct_table_entries = ["9999", "Detector 3", "Al , C"]
        # Create source tables
        likelyhood_table = CreateEmptyTableWorkspace(OutputWorkspace="likelyhood")
        likelyhood_table.addColumn("str", "Element")
        likelyhood_table.addColumn("int", "Likelihood")
        likelyhood_table.addRow(["Al", 20])
        likelyhood_table.addRow(["C", 18])

        # Run function
        self.model.update_match_table("likelyhood", "9999; Detector 3")

        # Assert statements
        self.assertEqual(self.model.table_entries.get(), correct_table_entries)

        # Delete tables from ADS
        self.delete_if_present("likelyhood")

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_find_peak_algorithm")
    @mock.patch(
        "Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_peak_matching_algorithm")
    def test_run_peak_algorithms_with_workspace(self, mock_peak_matching, mock_run_find_peak, mock_retrieve_ws):
        # setup
        parameters = {"workspace": "9999; Detector 1"}
        mock_retrieve_ws.return_value = "mock_group"

        # Run function
        self.model._run_peak_algorithms(parameters)

        # Assert statements
        mock_retrieve_ws.assert_called_with("9999")
        mock_run_find_peak.assert_called_with(parameters, "mock_group")
        mock_peak_matching.assert_called_with("9999; Detector 1", "mock_group")

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_find_peak_algorithm")
    @mock.patch(
        "Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_peak_matching_algorithm")
    def test_run_peak_algorithms_with_group(self, mock_peak_matching, mock_run_find_peak, mock_retrieve_ws):
        # setup
        self.group_context = mock.Mock()
        mock_group = mock.Mock()
        mock_group.getNames.return_value = ["9999; Detector 1", "9999; Detector 2"]
        parameters = {"workspace": "9999"}
        mock_retrieve_ws.return_value = mock_group
        mock_run_find_peak.return_value = False

        # Run function
        self.model._run_peak_algorithms(parameters)

        # Assert statements
        mock_retrieve_ws.assert_called_with("9999")
        mock_run_find_peak.assert_has_calls([mock.call({'workspace': '9999; Detector 1'}, mock_group, True),
                                             mock.call({'workspace': '9999; Detector 2'}, mock_group, True)])
        mock_peak_matching.assert_has_calls([mock.call("9999; Detector 1", mock_group),
                                             mock.call("9999; Detector 2", mock_group)])

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_find_peak_algorithm")
    @mock.patch(
        "Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel._run_peak_matching_algorithm")
    def test_run_peak_algorithms_with_group_and_peak_matching_not_run(self, mock_peak_matching, mock_run_find_peak,
                                                                      mock_retrieve_ws):
        # setup
        mock_group = mock.Mock()
        mock_group.getNames.return_value = ['9999; Detector 3', '9999; Detector 4']
        parameters = {"workspace": "9999"}
        mock_retrieve_ws.return_value = mock_group
        mock_run_find_peak.return_value = True

        # Run function
        self.model._run_peak_algorithms(parameters)

        # Assert statements
        mock_retrieve_ws.assert_called_with("9999")
        mock_run_find_peak.assert_has_calls([mock.call({'workspace': '9999; Detector 3'}, mock_group, True),
                                             mock.call({'workspace': '9999; Detector 4'}, mock_group, True)])
        mock_peak_matching.assert_not_called()

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.remove_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.find_peak_algorithm")
    def test_run_find_peak_algo_with_default_peaks_and_no_errors(self, mock_find_peaks, mock_remove_ws,
                                                                 mock_retrieve_ws):
        # setup
        workspace = "9999; Detector 1"
        number_of_peaks = 3
        mock_group = mock.Mock()
        mock_group.name.return_value = "9999"
        mock_table = mock.Mock()
        mock_table.rowCount.return_value = number_of_peaks
        mock_retrieve_ws.return_value = mock_table

        param = {"workspace": workspace, "min_energy": 10, "max_energy": 1000, "threshold": 1, "default_width": True}
        ignore_peak_matching = self.model._run_find_peak_algorithm(param, mock_group)

        # Assert statements
        mock_find_peaks.assert_called_once_with(workspace, 3, 10, 1000, 1, 0.5, 1, 2.5)
        mock_group.add.assert_called_once_with(workspace + "_peaks")
        self.assertEqual(mock_table.rowCount.call_count, 1)
        self.assertEqual(self.model.current_peak_table_info["workspace"], workspace)
        self.assertEqual(self.model.current_peak_table_info["number_of_peaks"], number_of_peaks)
        self.assertEqual(mock_retrieve_ws.call_count, 1)
        self.assertEqual(mock_remove_ws.call_count, 2)
        self.assertEqual(ignore_peak_matching, False)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.remove_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.find_peak_algorithm")
    def test_run_find_peak_algo_without_default_peaks_and_no_errors(self, mock_find_peaks, mock_remove_ws,
                                                                    mock_retrieve_ws):
        # setup
        workspace = "9999; Detector 2"
        number_of_peaks = 5
        mock_group = mock.Mock()
        mock_group.name.return_value = "9999"
        mock_table = mock.Mock()
        mock_table.rowCount.return_value = number_of_peaks
        mock_retrieve_ws.return_value = mock_table

        param = {"workspace": workspace, "min_energy": 1, "max_energy": 200, "threshold": 4, "default_width": False,
                 "min_width": 0.3, "max_width": 2, "estimate_width": 1.5}
        ignore_peak_matching = self.model._run_find_peak_algorithm(param, mock_group)

        # Assert statements
        mock_find_peaks.assert_called_once_with(workspace, 3, 1, 200, 4, 0.3, 1.5, 2)
        mock_group.add.assert_called_once_with(workspace + "_peaks")
        self.assertEqual(mock_table.rowCount.call_count, 1)
        self.assertEqual(self.model.current_peak_table_info["workspace"], workspace)
        self.assertEqual(self.model.current_peak_table_info["number_of_peaks"], number_of_peaks)
        self.assertEqual(mock_retrieve_ws.call_count, 1)
        self.assertEqual(mock_remove_ws.call_count, 2)
        self.assertEqual(ignore_peak_matching, False)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.remove_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.find_peak_algorithm")
    def test_run_find_peak_algo_with_default_peaks_and_errors(self, mock_find_peaks, mock_remove_ws, mock_retrieve_ws):
        # setup
        workspace = "9999; Detector 3"
        number_of_peaks = 0
        mock_group = mock.Mock()
        mock_table = mock.Mock()
        mock_table.name.return_value = "mock_value"
        mock_table.rowCount.return_value = number_of_peaks
        mock_retrieve_ws.return_value = mock_table

        param = {"workspace": workspace, "min_energy": 50, "max_energy": 4000, "threshold": 1.5, "default_width": True}
        with self.assertRaises(RuntimeError):
            self.model._run_find_peak_algorithm(param, mock_group)

        # Assert statements
        mock_find_peaks.assert_called_once_with(workspace, 3, 50, 4000, 1.5, 0.1, 0.5, 1.5)
        self.assertEqual(mock_table.rowCount.call_count, 1)
        self.assertEqual(mock_table.delete.call_count, 1)
        self.assertEqual(self.model.current_peak_table_info["workspace"], workspace)
        self.assertEqual(self.model.current_peak_table_info["number_of_peaks"], number_of_peaks)
        self.assertEqual(mock_retrieve_ws.call_count, 1)
        self.assertEqual(mock_remove_ws.call_count, 2)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.remove_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.find_peak_algorithm")
    def test_run_find_peak_algo_with_default_peaks_and_delay_errors(self, mock_find_peaks, mock_remove_ws,
                                                                    mock_retrieve_ws):
        # setup
        workspace = "9999; Detector 4"
        number_of_peaks = 0
        mock_group = mock.Mock()
        mock_table = mock.Mock()
        mock_table.rowCount.return_value = number_of_peaks
        mock_retrieve_ws.return_value = mock_table

        param = {"workspace": workspace, "min_energy": 5, "max_energy": 100, "threshold": 1.5, "default_width": True}
        ignore_peak_matching = self.model._run_find_peak_algorithm(param, mock_group, True)

        # Assert statements
        mock_find_peaks.assert_called_once_with(workspace, 3, 5, 100, 1.5, 0.1, 0.7, 1.5)
        self.assertEqual(mock_table.rowCount.call_count, 1)
        self.assertEqual(mock_table.delete.call_count, 1)
        self.assertEqual(self.model.current_peak_table_info["workspace"], workspace)
        self.assertEqual(self.model.current_peak_table_info["number_of_peaks"], number_of_peaks)
        self.assertEqual(mock_retrieve_ws.call_count, 1)
        self.assertEqual(mock_remove_ws.call_count, 2)
        self.assertEqual(ignore_peak_matching, True)
        self.assertEqual(self.model.warnings.get(), f"No peaks found in {workspace} try reducing acceptance threshold")

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.peak_matching_algorithm")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.GroupWorkspaces")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model.EAAutoTabModel.update_match_table")
    def test_run_peak_matching_algo(self, mock_update_match_table, mock_group_workspaces, mock_peak_matching):
        # setup
        mock_group = mock.Mock()
        mock_group.name.return_value = "9999"
        workspace = "9999; Detector 4"
        match_table_names = [workspace + "_all_matches", workspace + "_primary_matches",
                             workspace + "_secondary_matches",
                             workspace + "_all_matches_sorted_by_energy", workspace + "_likelihood"]

        self.model._run_peak_matching_algorithm(workspace, mock_group)

        mock_peak_matching.assert_called_once_with(workspace, match_table_names)
        mock_group_workspaces.assert_called_once_with(InputWorkspaces=match_table_names,
                                                      OutputWorkspace=workspace + "_matches")
        mock_group.add.assert_called_once_with(workspace + "_matches")
        mock_update_match_table.assert_called_once_with(workspace + "_likelihood", workspace)


if __name__ == '__main__':
    unittest.main()
