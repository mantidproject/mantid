import unittest
import sys
import six

if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock

from PyQt4 import QtGui
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_model import GroupingTableModel
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_view import GroupingTableView
from Muon.GUI.Common.grouping_table_widget.grouping_table_widget_presenter import GroupingTablePresenter
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common import mock_widget


class GroupingTablePresenterTest(unittest.TestCase):

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonDataContext()
        self.model = GroupingTableModel(data=self.data)
        self.view = GroupingTableView(parent=self.obj)
        self.presenter = GroupingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.group_names), 0)
        self.assertEqual(len(self.model.groups), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def add_three_groups_to_table(self):
        group1 = MuonGroup(group_name="my_group_0", detector_ids=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_ids=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_ids=[3])
        self.presenter.add_group(group1)
        self.presenter.add_group(group2)
        self.presenter.add_group(group3)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Initialization
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_table_has_three_columns_when_initialized(self):
        self.assertEqual(self.view.num_cols(), 3)

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Adding and removing groups
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_add_group_button_adds_group(self):
        self.presenter.handle_add_group_button_clicked()
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.groups), 1)

    def test_that_remove_group_button_removes_group(self):
        self.add_three_groups_to_table()
        self.presenter.handle_remove_group_button_clicked()
        self.assertEqual(self.view.num_rows(), 2)

    def test_that_add_group_button_adds_group_to_end_of_table(self):
        self.add_three_groups_to_table()

        self.presenter.add_group(MuonGroup(group_name="new", detector_ids=[4]))

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "new")

    def test_that_remove_group_button_removes_group_from_end_of_table(self):
        self.add_three_groups_to_table()

        self.presenter.handle_remove_group_button_clicked()

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "my_group_1")

    def test_that_highlighting_rows_and_clicking_remove_group_removes_the_selected_rows(self):
        self.add_three_groups_to_table()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.presenter.handle_remove_group_button_clicked()

        self.assertEqual(self.view.get_table_item_text(0, 0), "my_group_1")
        self.assertEqual(self.model.group_names, ["my_group_1"])

    def test_that_cannot_add_more_than_20_rows(self):
        for i in range(21):
            self.presenter.handle_add_group_button_clicked()

        self.assertEqual(self.view.num_rows(), 20)
        self.assertEqual(len(self.model.groups), 20)

    def test_that_trying_to_add_a_20th_row_gives_warning_message(self):
        for i in range(21):
            self.presenter.handle_add_group_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_remove_group_when_table_is_empty_does_not_throw(self):

        self.presenter.handle_remove_group_button_clicked()

    # ------------------------------------------------------------------------------------------------------------------
    # Testing context menu
    # ------------------------------------------------------------------------------------------------------------------

    def test_context_menu_add_grouping_with_no_rows_selected_adds_group_to_end_of_table(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.contextMenuEvent(0)
        self.view.add_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 2)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(self.view.get_table_item_text(1, 0), "group_2")

    def test_context_menu_add_grouping_with_rows_selected_does_not_add_group(self):
        self.presenter.handle_add_group_button_clicked()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0])

        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.add_group_action.isEnabled())

    def test_context_menu_remove_grouping_with_no_rows_selected_removes_last_row(self):
        for i in range(3):
            # names : group_1, group_2, group_3
            self.presenter.handle_add_group_button_clicked()

        self.view.contextMenuEvent(0)
        self.view.remove_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 2)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(self.view.get_table_item_text(0, 0), "group_1")
        self.assertEqual(self.view.get_table_item_text(1, 0), "group_2")

    def test_context_menu_remove_grouping_removes_selected_rows(self):
        for i in range(3):
            # names : group_1, group_2, group_3
            self.presenter.handle_add_group_button_clicked()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.view.contextMenuEvent(0)
        self.view.remove_group_action.triggered.emit(True)

        self.assertEqual(len(self.model.groups), 1)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(self.view.get_table_item_text(0, 0), "group_2")

    def test_context_menu_remove_grouping_disabled_if_no_groups_in_table(self):
        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.remove_group_action.isEnabled())

    def test_context_menu_add_pair_disabled_unless_two_groups_selected(self):
        self.add_three_groups_to_table()

        for selected_rows in [[], [0], [0, 1, 2]]:
            self.view._get_selected_row_indices = mock.Mock(return_value=selected_rows)
            self.view.contextMenuEvent(0)

            self.assertFalse(self.view.add_pair_action.isEnabled())


    # ------------------------------------------------------------------------------------------------------------------
    # Group name validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_can_change_group_name_to_valid_name_and_update_view_and_model(self):
        self.add_three_groups_to_table()

        self.view.grouping_table.item(0, 0).setText("new_name")

        self.assertEqual(self.view.get_table_item_text(0, 0), "new_name")
        self.assertIn("new_name", self.model.group_names)

    def test_that_if_invalid_name_given_warning_message_is_shown(self):
        self.add_three_groups_to_table()
        print(self.view.get_table_contents())
        invalid_names = ["", "@", "name!", "+-"]

        for invalid_name in invalid_names:
            self.view.grouping_table.item(0, 0).setText(invalid_name)
            print(self.view.get_table_contents())

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), "my_group_0")
            self.assertIn("my_group_0", self.model.group_names)

    def test_that_group_names_with_numbers_and_letters_and_underscores_are_valid(self):
        self.add_three_groups_to_table()

        valid_names = ["fwd", "fwd_1", "1234", "FWD0001", "_fwd"]

        for valid_name in valid_names:
            self.view.grouping_table.item(0, 0).setText(valid_name)

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), valid_name)
            self.assertIn(valid_name, self.model.group_names)

    def test_that_renaming_group_to_duplicate_fails_and_reverts_to_previous_value(self):
        self.add_three_groups_to_table()

        self.view.grouping_table.setCurrentCell(0, 0)
        self.view.grouping_table.item(0, 0).setText("my_group_1")

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "my_group_0")
        self.assertIn("my_group_0", self.model.group_names)

    def test_that_warning_shown_if_duplicated_group_name_used(self):
        self.add_three_groups_to_table()

        self.view.grouping_table.setCurrentCell(0, 0)
        self.view.grouping_table.item(0, 0).setText("my_group_1")

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_default_group_name_is_group_1(self):
        self.presenter.handle_add_group_button_clicked()

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "group_1")
        self.assertIn("group_1", self.model.group_names)

    def test_that_adding_new_group_creates_incremented_default_name(self):
        self.presenter.handle_add_group_button_clicked()
        self.presenter.handle_add_group_button_clicked()
        self.presenter.handle_add_group_button_clicked()

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "group_1")
        self.assertEqual(str(self.view.get_table_item_text(1, 0)), "group_2")
        self.assertEqual(str(self.view.get_table_item_text(2, 0)), "group_3")
        six.assertCountEqual(self, self.model.group_names, ["group_1", "group_2", "group_3"])

    # ------------------------------------------------------------------------------------------------------------------
    # detector ID validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_if_not_entering_numbers_into_detector_IDs_the_changes_are_rejected(self):
        self.add_three_groups_to_table()

        invalid_id_lists = ["fwd", "a", "A", "!", "_", "(1)", "11a22"]

        call_count = 0
        for invalid_ids in invalid_id_lists:
            call_count += 1

            self.view.grouping_table.setCurrentCell(0, 1)
            self.view.grouping_table.item(0, 1).setText(invalid_ids)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

            self.assertEqual(self.view.get_table_item_text(0, 1), "1")
            self.assertEqual(self.model._data._groups["my_group_0"].detectors, [1])

    def test_that_displayed_values_are_simplified_to_least_verbose_form(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.grouping_table.setCurrentCell(0, 1)
        self.view.grouping_table.item(0, 1).setText("20-25,10,5,4,3,2,1")

        self.assertEqual(self.view.get_table_item_text(0, 1), "1-5,10,20-25")
        self.assertEqual(self.model._data._groups["group_1"].detectors, [1, 2, 3, 4, 5, 10, 20, 21, 22, 23, 24, 25])

    def test_that_if_detector_list_changed_that_number_of_detectors_updates(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.grouping_table.setCurrentCell(0, 1)
        self.view.grouping_table.item(0, 1).setText("1-10")

        self.assertEqual(self.view.get_table_item_text(0, 2), "10")

    def test_that_detector_numbers_cannot_be_edited(self):
        self.presenter.handle_add_group_button_clicked()

        self.view.grouping_table.setCurrentCell(0, 2)
        self.view.grouping_table.item(0, 2).setText("25")

        self.assertEqual(self.view.get_table_item_text(0, 2), "1")


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
