# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import six
from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter import GroupingTabPresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_view import GroupingTabView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter, MuonGroup
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter, MuonPair
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView


class GroupingTabPresenterTest(GuiTest):
    def setUp(self):
        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext(self.data_context.check_group_contains_valid_detectors)
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)

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

        self.assertEqual(self.pairing_table_view.num_rows(), 3)

    def test_context_menu_add_pair_adds_correct_pair_if_two_groups_selected(self):
        self.grouping_table_view._get_selected_row_indices = mock.Mock(return_value=[0, 1])
        self.grouping_table_view.contextMenuEvent(0)
        self.grouping_table_view.add_pair_action.triggered.emit(True)

        pair_name = "pair_0"

        self.assertEqual(self.group_context[pair_name].forward_group, "fwd")
        self.assertEqual(self.group_context[pair_name].backward_group, "bwd")

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
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value="grouping.xml")
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 10])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]

        with mock.patch(
                "Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter.xml_utils.load_grouping_from_XML") as mock_load:
            # mock the loading to return set groups/pairs
            mock_load.return_value = (groups, pairs, 'description', 'pair1')
            self.view.load_grouping_button.clicked.emit(True)

            six.assertCountEqual(self, self.model.group_names, ["grp1", "grp2"])
            six.assertCountEqual(self, self.model.pair_names, ["pair1"])
            self.assertEqual(self.grouping_table_view.num_rows(), 2)
            self.assertEqual(self.pairing_table_view.num_rows(), 1)
            self.assertEqual(self.pairing_table_view.pairing_table.cellWidget(0, 1).currentText(), "grp1")
            self.assertEqual(self.pairing_table_view.pairing_table.cellWidget(0, 2).currentText(), "grp2")

    def test_loading_does_not_insert_invalid_groups(self):
        self.view.show_file_browser_and_return_selection = mock.Mock(return_value="grouping.xml")
        groups = [MuonGroup(group_name="grp1", detector_ids=[1, 2, 3, 4, 5]),
                  MuonGroup(group_name="grp2", detector_ids=[6, 7, 8, 9, 1000])]
        pairs = [MuonPair(pair_name="pair1", forward_group_name="grp1", backward_group_name="grp2")]
        with mock.patch(
                "Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_presenter.xml_utils.load_grouping_from_XML") as mock_load:
            # mock the loading to return set groups/pairs
            mock_load.return_value = (groups, pairs, 'description', 'pair1')
            self.view.load_grouping_button.clicked.emit(True)

            self.view.display_warning_box.assert_called_once_with('Invalid detectors in group grp2')
            six.assertCountEqual(self, self.model.group_names, ["grp1"])
            six.assertCountEqual(self, self.model.pair_names, [])
            self.assertEqual(self.grouping_table_view.num_rows(), 1)
            self.assertEqual(self.pairing_table_view.num_rows(), 0)

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


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
