# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from mantid import ConfigService
from mantid.api import FileFinder
from qtpy.QtWidgets import QWidget
import Muon.GUI.Common.utilities.load_utils as load_utils

from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter, MuonGroup
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter, MuonPair
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


def pair_name():
    name = []
    for i in range(21):
        name.append("pair_" + str(i + 1))
    return name


#
def perform_musr_file_finder(self):
    ConfigService['default.instrument'] = 'MUSR'
    file_path = FileFinder.findRuns('MUSR00022725.nxs')[0]
    ws, run, filename, psi_data = load_utils.load_workspace_from_filename(file_path)
    self.assert_(not psi_data)
    self.data_context._loaded_data.remove_data(run=run)
    self.data_context._loaded_data.add_data(run=[run], workspace=ws, filename=filename, instrument='MUSR')
    self.data_context.current_runs = [[22725]]

    self.context.data_context._instrument = "MUSR"
    self.context.update_current_data()
    test_pair = MuonPair('test_pair', 'top', 'bottom', alpha=0.75)
    self.group_context.add_pair(pair=test_pair)
    self.presenter.update_view_from_model()


@start_qapplication
class GroupingTabPresenterTest(unittest.TestCase):
    def setUp(self):
        self.obj = QWidget()

        self.loaded_data = MuonLoadData()

        setup_context_for_tests(self)

        self.model = GroupingTabModel(context=self.context)

        self.grouping_table_view = GroupingTableView()
        self.grouping_table_widget = GroupingTablePresenter(self.grouping_table_view, self.model)

        self.pairing_table_view = PairingTableView()
        self.pairing_table_widget = PairingTablePresenter(self.pairing_table_view, self.model)

        self.grouping_table_view.warning_popup = mock.MagicMock()
        self.pairing_table_view.warning_popup = mock.MagicMock()

        self.add_three_groups()
        self.add_two_pairs()

        self.view = GroupingTabView(self.grouping_table_view, self.pairing_table_view)
        self.presenter = GroupingTabPresenter(self.view, self.model,
                                              self.grouping_table_widget,
                                              self.pairing_table_widget)

        self.presenter.create_update_thread = mock.MagicMock(return_value=mock.MagicMock())
        self.presenter.pairing_table_widget.handle_add_pair_button_clicked = mock.MagicMock()
        self.view.display_warning_box = mock.MagicMock()
        self.grouping_table_view.warning_popup = mock.MagicMock()
        self.pairing_table_view.warning_popup = mock.MagicMock()

    def add_three_groups(self):
        testgroup1 = MuonGroup(group_name="fwd", detector_ids=[1, 2, 3, 4, 5])
        testgroup2 = MuonGroup(group_name="bwd", detector_ids=[6, 7, 8, 9, 10])
        testgroup3 = MuonGroup(group_name="top", detector_ids=[11, 12, 13, 14, 15])
        self.grouping_table_widget.add_group(testgroup1)
        self.grouping_table_widget.add_group(testgroup2)
        self.grouping_table_widget.add_group(testgroup3)

    def add_two_pairs(self):
        testpair1 = MuonPair(pair_name="long1", forward_group_name="fwd", backward_group_name="bwd")
        testpair2 = MuonPair(pair_name="long2", forward_group_name="fwd", backward_group_name="top")
        self.pairing_table_widget.add_pair(testpair1)
        self.pairing_table_widget.add_pair(testpair2)

    def tearDown(self):
        self.obj = None

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS
    # ------------------------------------------------------------------------------------------------------------------
    def test_context_menu_add_pair_adds_pair_if_two_groups_selected(self):
        self.assertEqual(self.pairing_table_view.num_rows(), 2)
        self.grouping_table_view._get_selected_row_indices = mock.Mock(return_value=[0, 1])
        self.grouping_table_view.contextMenuEvent(0)
        self.grouping_table_view.add_pair_action.triggered.emit(True)

        self.presenter.pairing_table_widget.handle_add_pair_button_clicked.assert_called_once_with('fwd', 'bwd')

    def test_that_clear_button_clears_model_and_view(self):
        self.view.clear_grouping_button.clicked.emit(True)

        self.assertEqual(len(self.model.groups), 0)
        self.assertEqual(len(self.model.pairs), 0)
        self.assertEqual(self.grouping_table_view.num_rows(), 0)
        self.assertEqual(self.pairing_table_view.num_rows(), 0)

    @mock.patch("Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter.xml_utils.load_grouping_from_XML")
    def test_that_load_grouping_triggers_the_correct_function(self, mock_load):
        self.view.show_file_browser_and_return_selection = mock.MagicMock(return_value="grouping.xml")
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]
        mock_load.return_value = (groups, pairs, 'description', 'pair1')

        self.view.load_grouping_button.clicked.emit(True)

        self.assertEqual(mock_load.call_count, 1)
        self.assertEqual(mock_load.call_args[0][0], "grouping.xml")

    def test_that_load_grouping_inserts_loaded_groups_and_pairs_correctly(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='pair1')

        self.assertCountEqual(self.model.group_names, ["grp1", "grp2"])
        self.assertCountEqual(self.model.pair_names, ["pair1"])
        self.assertEqual(self.grouping_table_view.num_rows(), 2)
        self.assertEqual(self.pairing_table_view.num_rows(), 1)
        self.assertEqual(self.pairing_table_view.pairing_table.cellWidget(0, 2).currentText(), "grp1")
        self.assertEqual(self.pairing_table_view.pairing_table.cellWidget(0, 3).currentText(), "grp2")

    def test_loading_does_not_insert_invalid_groups(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 1000])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='pair1')

        self.view.display_warning_box.assert_called_once_with('Invalid detectors in group grp2')
        self.assertCountEqual(self.model.group_names, ["grp1"])
        self.assertCountEqual(self.model.pair_names, [])
        self.assertEqual(self.grouping_table_view.num_rows(), 1)
        self.assertEqual(self.pairing_table_view.num_rows(), 0)

    def test_loading_selects_all_pairs_if_any_pairs_exist_and_no_default_set(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='')

        self.assertEqual(self.model.selected_pairs, ["pair1"])
        self.assertEqual(self.model.selected_groups, [])

    def test_loading_selects_groups_if_no_pairs_exist_and_no_default_set(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = []

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='')

        self.assertEqual(self.model.selected_pairs, [])
        self.assertEqual(self.model.selected_groups, ["grp1", "grp2"])

    def test_loading_selects_default_pairs_and_groups_correctly(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='grp2')

        self.assertEqual(self.model.selected_pairs, [])
        self.assertEqual(self.model.selected_groups, ["grp2"])

    def test_loading_selects_correctly_when_default_is_invalid(self):
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        self._run_handle_load_grouping_with_mocked_load(groups, pairs, default='grp3')

        self.assertEqual(self.model.selected_pairs, ["pair1"])
        self.assertEqual(self.model.selected_groups, [])

    def test_that_save_grouping_triggers_the_correct_function(self):
        # Save functionality is tested elsewhere
        self.view.show_file_save_browser_and_return_selection = mock.Mock(return_value="grouping.xml")

        with mock.patch(
                "Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter.xml_utils.save_grouping_to_XML") as mock_save:
            self.view.save_grouping_button.clicked.emit(True)

            self.assertEqual(mock_save.call_count, 1)
            self.assertEqual(mock_save.call_args[0][-1], "grouping.xml")

    def test_update_all_calculates_groups_and_pairs(self):
        self.presenter.handle_update_all_clicked()

        self.presenter.update_thread.threadWrapperSetUp.assert_called_once_with(self.presenter.disable_editing,
                                                                                self.presenter.handle_update_finished,
                                                                                self.presenter.error_callback)
        self.presenter.update_thread.start.assert_called_once_with()

    def test_removing_group_removes_linked_pairs(self):
        self.group_context.clear_pairs()
        self.group_context.clear_groups()
        self.add_three_groups()
        self.add_two_pairs()

        self.presenter.grouping_table_widget.remove_last_row_in_view_and_model()

        self.assertEqual(self.model.pair_names, ['long1'])

    def test_that_adding_pair_with_context_menu_allows_for_name_specification(self):
        self.presenter.add_pair_from_grouping_table("first", "second")
        self.pairing_table_widget.handle_add_pair_button_clicked.assert_called_once_with("first", "second")

    def _run_handle_load_grouping_with_mocked_load(self, groups, pairs, description='description', default=''):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value="grouping.xml")
        with mock.patch(
                "Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter.xml_utils.load_grouping_from_XML") as mock_load:
            # mock the loading to return set groups/pairs
            mock_load.return_value = (groups, pairs, 'description', default)
            self.presenter.handle_load_grouping_from_file()

    def test_default_clicked(self):
        self.presenter._model.reset_groups_and_pairs_to_default = mock.MagicMock(return_value="success")
        self.presenter.handle_default_grouping_button_clicked()
        self.assertEqual(self.view.display_warning_box.call_count, 0)

    def test_default_clicked_no_data(self):
        self.presenter._model.reset_groups_and_pairs_to_default = mock.MagicMock(return_value="failed")
        self.presenter.handle_default_grouping_button_clicked()
        self.assertEqual(self.view.display_warning_box.call_count, 1)

    def test_update_description_to_empty_on_clear_all(self):
        self.presenter.on_clear_requested()
        self.assertEqual('', self.view.get_description_text())


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
