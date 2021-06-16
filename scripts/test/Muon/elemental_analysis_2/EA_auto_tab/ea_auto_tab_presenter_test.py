import unittest
import unittest.mock as mock
from copy import deepcopy
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter import EAAutoTabPresenter
from Muon.GUI.ElementalAnalysis2.context.context import ElementalAnalysisContext
from Muon.GUI.ElementalAnalysis2.context.ea_group_context import EAGroupContext
from Muon.GUI.ElementalAnalysis2.ea_group import EAGroup
from Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_model import PEAKS_WS_SUFFIX, MATCH_GROUP_WS_SUFFIX


class EAAutoTabPresenterTest(unittest.TestCase):

    def setUp(self):
        workspaces = ["9999; Detector 1", "9999; Detector 2", "9999; Detector 3", "9999; Detector 4"]
        self.group_context = EAGroupContext()
        for group_name in workspaces:
            group = EAGroup(group_name, group_name.split(";")[1].strip(),
                            group_name.split(";")[0].strip())
            group.update_peak_table("9999", group_name + PEAKS_WS_SUFFIX)
            group.update_matches_table("9999", group_name + MATCH_GROUP_WS_SUFFIX)
            self.group_context.add_group(group)

        self.context = ElementalAnalysisContext(None, self.group_context)
        self.presenter = EAAutoTabPresenter(self.context, mock.Mock(), mock.Mock(), mock.Mock())

    def test_run_find_peak_algorithms_if_parameters_is_None(self):
        self.presenter.view.get_parameters_for_find_peaks.return_value = None

        self.presenter.run_find_peak_algorithms()

        self.presenter.model.handle_peak_algorithms.assert_not_called()

    def test_run_find_peak_algorithms_if_parameters_is_valid(self):
        self.presenter.view.get_parameters_for_find_peaks.return_value = 3

        self.presenter.run_find_peak_algorithms()

        self.presenter.model.handle_peak_algorithms.assert_called_once_with(3)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.EAAutoPopupTable")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.check_if_workspace_exist")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.retrieve_ws")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.EAAutoTabPresenter.extract_rows")
    def test_show_table(self, mock_extract_row, mock_retrieve_ws, mock_check_workspace, mock_ea_auto_popup_table):
        # setup
        mock_extract_row.return_value = ["row1", "row2"]
        mock_popup = mock.Mock()
        mock_ea_auto_popup_table.return_value = mock_popup
        mock_table = mock.Mock()
        mock_table.getColumnNames.return_value = ["column1", "column2"]
        mock_retrieve_ws.return_value = mock_table
        mock_check_workspace.return_value = True

        self.presenter.show_table("mock_table")

        # Assert statement
        mock_extract_row.assert_called_once_with("mock_table")
        mock_popup.create_table.assert_called_once_with(["column1", "column2"])
        mock_popup.add_entry_to_table.assert_has_calls([mock.call("row1"), mock.call("row2")])
        mock_popup.show.assert_called_once()

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.message_box.warning")
    def test_show_table_with_invalid_string(self, mock_warning):
        self.presenter.show_table("mock_table")

        # Assert statement
        mock_warning.assert_called_once_with("ERROR : mock_table Table does not exist", None)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.check_if_workspace_exist")
    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.message_box.warning")
    def test_show_table_with_empty_table(self, mock_warning, mock_check_workspace):
        # setup
        mock_check_workspace.return_value = False

        self.presenter.show_table("")

        # Assert statement
        mock_warning.assert_called_once_with("ERROR : No selected table", None)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.retrieve_ws")
    def test_extract_rows(self, mock_retrieve_ws):
        # setup
        mock_table = mock.Mock()
        mock_table.rowCount.return_value = 3
        mock_table.toDict.return_value = {"fake_column1": ["spam", "eggs", "ham"], "fake_column2": [10, 15, 20],
                                          "fake_column3": [100, 2000, 345]}
        correct_table_entries = [["spam", "10", "100"], ["eggs", "15", "2000"], ["ham", "20", "345"]]
        mock_retrieve_ws.return_value = mock_table

        table_entries = self.presenter.extract_rows("mock_table_name")

        # Assert statement
        mock_retrieve_ws.assert_called_once_with("mock_table_name")
        self.assertEqual(correct_table_entries, table_entries)

    @mock.patch("Muon.GUI.ElementalAnalysis2.auto_widget.ea_auto_tab_presenter.retrieve_ws")
    def test_update_view(self, mock_retrieve_ws):
        mock_group = mock.Mock()
        get_names_return_value = ["test;mock_refitted_peaks", "test;mock_peaks", "test;mock_matches",
                                  "test;mock_workspace",
                                  "test;fake_refitted_peaks", "test;fake_peaks", "test;fake_matches",
                                  "test;fake_workspace"]

        mock_group.getNames = lambda: deepcopy(get_names_return_value)
        mock_retrieve_ws.return_value = mock_group

        self.presenter.model.current_peak_table_info = {"workspace": "mock_peak_info", "number_of_peaks": 0}

        self.presenter.update_view()
        # Assert statement
        self.presenter.view.add_options_to_find_peak_combobox.assert_called_once_with({'9999': ['All', 'Detector 1',
                                                                                                'Detector 2',
                                                                                                'Detector 3',
                                                                                                'Detector 4']})

        self.presenter.view.add_options_to_show_peak_combobox.assert_called_once_with({'9999':
                                                                                           ['9999; Detector 1_peaks',
                                                                                            '9999; Detector 2_peaks',
                                                                                            '9999; Detector 3_peaks',
                                                                                            '9999; Detector 4_peaks']})

        self.presenter.view.add_options_to_show_matches_combobox.assert_called_once_with({'9999':
                                                                                          get_names_return_value * 4})

        self.presenter.view.set_peak_info.assert_called_once_with(workspace="mock_peak_info", number_of_peaks=0)


if __name__ == '__main__':
    unittest.main()
