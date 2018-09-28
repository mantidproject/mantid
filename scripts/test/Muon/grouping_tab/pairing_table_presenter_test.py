import unittest
import sys
import six

if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock

from PyQt4 import QtGui
from PyQt4.QtGui import QApplication

from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_model import PairingTableModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_context import MuonContext

# global QApplication (get errors if > 1 instance in the code)
QT_APP = QApplication([])


class PairingTablePresenterTest(unittest.TestCase):

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonContext()
        self.add_three_groups_to_model()

        self.model = GroupingTabModel(data=self.data)
        self.view = PairingTableView(parent=self.obj)
        self.presenter = PairingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.pair_names), 0)
        self.assertEqual(len(self.model.pairs), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def add_three_groups_to_model(self):
        group1 = MuonGroup(group_name="my_group_0", detector_IDs=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_IDs=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_IDs=[3])
        self.data.add_group(group1)
        self.data.add_group(group2)
        self.data.add_group(group3)

    def add_two_pairs_to_table(self):
        pair1 = MuonPair(pair_name="my_pair_0", forward_group_name="my_group_0", backward_group_name="my_group_1", alpha=1.0)
        pair2 = MuonPair(pair_name="my_pair_1", forward_group_name="my_group_1", backward_group_name="my_group_2", alpha=1.0)
        self.presenter.add_pair(pair1)
        self.presenter.add_pair(pair2)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Initialization
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_table_has_five_columns_when_initialized(self):
        # these are : pair name, group 1, group 2, alpha, guess alpha
        self.assertEqual(self.view.num_cols(), 5)

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Adding and removing groups
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_add_pair_button_adds_pair(self):
        self.presenter.handle_add_pair_button_clicked()
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(len(self.model.pairs), 1)

    def test_that_remove_pair_button_removes_group(self):
        self.add_two_pairs_to_table()
        self.presenter.handle_remove_pair_button_clicked()
        self.assertEqual(self.view.num_rows(), 1)

    def test_that_add_pair_button_adds_pair_to_end_of_table(self):
        self.add_two_pairs_to_table()

        self.presenter.add_pair(MuonPair(pair_name="new"))

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "new")

    def test_that_remove_pair_button_removes_pair_from_end_of_table(self):
        self.add_two_pairs_to_table()

        self.presenter.handle_remove_pair_button_clicked()

        self.assertEqual(self.view.get_table_item_text(self.view.num_rows() - 1, 0), "my_pair_0")

    def test_that_highlighting_rows_and_clicking_remove_pair_removes_the_selected_rows(self):
        self.add_two_pairs_to_table()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 1])

        self.presenter.handle_remove_pair_button_clicked()

        self.assert_model_empty()
        self.assert_view_empty()

    def test_that_cannot_add_more_than_20_rows(self):
        for i in range(21):
            self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.view.num_rows(), 20)
        self.assertEqual(len(self.model.pairs), 20)

    def test_that_trying_to_add_a_20th_row_gives_warning_message(self):
        for i in range(21):
            self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_remove_group_when_table_is_empty_does_not_throw(self):
        for i in range(3):
            self.presenter.handle_remove_pair_button_clicked()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Context menu has "add pair" and "remove pair" functionality
    # ------------------------------------------------------------------------------------------------------------------

    def test_context_menu_add_pairing_with_no_rows_selected_adds_pair_to_end_of_table(self):
        self.view.contextMenuEvent(0)
        self.view.add_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 1)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(self.view.get_table_item_text(0, 0), "pair_0")

    def test_context_menu_add_pairing_with_rows_selected_does_not_add_pair(self):
        self.add_two_pairs_to_table()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0])

        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.add_pair_action.isEnabled())

    def test_context_menu_remove_pairing_with_no_rows_selected_removes_last_row(self):
        for i in range(3):
            # names : pair_0, pair_1, pair_2
            self.presenter.handle_add_pair_button_clicked()

        self.view.contextMenuEvent(0)
        self.view.remove_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 2)
        self.assertEqual(self.view.num_rows(), 2)
        self.assertEqual(self.view.get_table_item_text(0, 0), "pair_0")
        self.assertEqual(self.view.get_table_item_text(1, 0), "pair_1")

    def test_context_menu_remove_pairing_removes_selected_rows(self):
        for i in range(3):
            # names : pair_0, pair_1, pair_2
            self.presenter.handle_add_pair_button_clicked()
        self.view._get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.view.contextMenuEvent(0)
        self.view.remove_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 1)
        self.assertEqual(self.view.num_rows(), 1)
        self.assertEqual(self.view.get_table_item_text(0, 0), "pair_1")

    def test_context_menu_remove_pairing_disabled_if_no_pairs_in_table(self):
        self.view.contextMenuEvent(0)

        self.assertFalse(self.view.remove_pair_action.isEnabled())

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Pair name validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_can_change_pair_name_to_valid_name_and_update_view_and_model(self):
        self.add_two_pairs_to_table()

        self.view.pairing_table.setCurrentCell(0, 0)
        self.view.pairing_table.item(0, 0).setText("new_name")

        self.assertEqual(self.view.get_table_item_text(0, 0), "new_name")
        self.assertIn("new_name", self.model.pair_names)

    def test_that_if_invalid_name_given_warning_message_is_shown(self):
        self.add_two_pairs_to_table()

        invalid_names = ["", "@", "name!", "+-"]
        call_count = self.view.warning_popup.call_count
        for invalid_name in invalid_names:
            call_count += 1
            self.view.pairing_table.setCurrentCell(0, 0)
            self.view.pairing_table.item(0, 0).setText(invalid_name)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

    def test_that_if_invalid_name_given_name_reverts_to_its_previous_value(self):
        self.add_two_pairs_to_table()

        invalid_names = ["", "@", "name!", "+-"]

        for invalid_name in invalid_names:
            self.view.pairing_table.setCurrentCell(0, 0)
            self.view.pairing_table.item(0, 0).setText(invalid_name)

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), "my_pair_0")
            self.assertIn("my_pair_0", self.model.pair_names)

    def test_that_pair_names_with_numbers_and_letters_and_underscores_are_valid(self):
        self.add_two_pairs_to_table()

        valid_names = ["fwd", "fwd_1", "1234", "FWD0001", "_fwd"]

        for valid_name in valid_names:
            self.view.pairing_table.setCurrentCell(0, 0)
            self.view.pairing_table.item(0, 0).setText(valid_name)

            self.assertEqual(str(self.view.get_table_item_text(0, 0)), valid_name)
            self.assertIn(valid_name, self.model.pair_names)

    def test_that_renaming_group_to_duplicate_fails_and_reverts_to_previous_value(self):
        self.add_two_pairs_to_table()

        self.view.pairing_table.setCurrentCell(0, 0)
        self.view.pairing_table.item(0, 0).setText("my_pair_1")

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "my_pair_0")
        self.assertIn("my_pair_0", self.model.pair_names)

    def test_that_warning_shown_if_duplicated_pair_name_used(self):
        self.add_two_pairs_to_table()

        self.view.pairing_table.setCurrentCell(0, 0)
        self.view.pairing_table.item(0, 0).setText("my_group_1")

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_default_pair_name_is_pair_0(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "pair_0")
        self.assertIn("pair_0", self.model.pair_names)

    def test_that_adding_new_pair_creates_incremented_default_name(self):
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(str(self.view.get_table_item_text(0, 0)), "pair_0")
        self.assertEqual(str(self.view.get_table_item_text(1, 0)), "pair_1")
        self.assertEqual(str(self.view.get_table_item_text(2, 0)), "pair_2")
        six.assertCountEqual(self, self.model.pair_names, ["pair_0", "pair_1", "pair_2"])


class GroupSelectorTest(unittest.TestCase):

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonContext()

        self.model = GroupingTabModel(data=self.data)
        self.view = PairingTableView(parent=self.obj)
        self.presenter = PairingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.pair_names), 0)
        self.assertEqual(len(self.model.pairs), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def add_three_groups_to_model(self):
        group1 = MuonGroup(group_name="my_group_0", detector_IDs=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_IDs=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_IDs=[3])
        self.data.add_group(group1)
        self.data.add_group(group2)
        self.data.add_group(group3)

    def add_two_pairs_to_table(self):
        pair1 = MuonPair(pair_name="my_pair_0", forward_group_name="my_group_0", backward_group_name="my_group_1", alpha=1.0)
        pair2 = MuonPair(pair_name="my_pair_1", forward_group_name="my_group_1", backward_group_name="my_group_2", alpha=1.0)
        self.presenter.add_pair(pair1)
        self.presenter.add_pair(pair2)

    def get_group_1_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 1)

    def get_group_2_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 2)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : test the functionality around the combo boxes which allow the user to select the two groups that
    #         together make the muon pair.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_adding_pair_when_no_groups_exist_leaves_combo_boxes_empty(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector(0).count(), 0)
        self.assertEqual(self.get_group_2_selector(0).count(), 0)
        self.assertEqual(self.get_group_1_selector(0).currentText(), "")
        self.assertEqual(self.get_group_2_selector(0).currentText(), "")

    def test_that_adding_pair_then_adding_group_puts_group_in_combos(self):
        self.presenter.handle_add_pair_button_clicked()
        self.data.add_group(MuonGroup(group_name="my_group_0", detector_IDs=[1]))
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector(0).count(), 1)
        self.assertEqual(self.get_group_2_selector(0).count(), 1)

    def test_that_adding_pair_then_adding_group_sets_combo_to_added_group(self):
        self.presenter.handle_add_pair_button_clicked()
        self.data.add_group(MuonGroup(group_name="my_group_0", detector_IDs=[1]))
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector(0).currentText(), "my_group_0")
        self.assertEqual(self.get_group_2_selector(0).currentText(), "my_group_0")

    def test_that_adding_two_groups_and_then_pair_sets_combo_to_added_groups(self):
        self.data.add_group(MuonGroup(group_name="my_group_0", detector_IDs=[1]))
        self.data.add_group(MuonGroup(group_name="my_group_1", detector_IDs=[2]))
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector(0).currentText(), "my_group_0")
        self.assertEqual(self.get_group_2_selector(0).currentText(), "my_group_1")

    def test_that_added_groups_appear_in_group_combo_boxes_in_new_pairs(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector(0).count(), 3)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_1"), -1)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_2"), -1)

    def test_that_added_groups_appear_in_group_combo_boxes_in_existing_pairs_if_update_called(self):
        self.presenter.handle_add_pair_button_clicked()
        self.add_three_groups_to_model()
        # the following method must be called
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector(0).count(), 3)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_1"), -1)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_2"), -1)

    def test_that_changing_group_selection_triggers_cell_changed_method_in_view(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()

        self.view.on_cell_changed = mock.Mock()
        self.get_group_1_selector(0).setCurrentIndex(1)

        self.assertEqual(self.view.on_cell_changed.call_count, 1)
        self.assertEqual(self.view.on_cell_changed.call_args_list[0][0], (0, 1))

    def test_that_removing_groups_and_then_calling_update_removes_groups_from_selections(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()
        del self.data.groups["my_group_1"]
        del self.data.groups["my_group_2"]
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector(0).count(), 1)
        self.assertEqual(self.get_group_2_selector(0).count(), 1)
        self.assertNotEqual(self.get_group_1_selector(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_2_selector(0).findText("my_group_0"), -1)


class AlphaTest(unittest.TestCase):

    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonContext()

        self.model = GroupingTabModel(data=self.data)
        self.view = PairingTableView(parent=self.obj)
        self.presenter = PairingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.pair_names), 0)
        self.assertEqual(len(self.model.pairs), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

    def add_three_groups_to_model(self):
        group1 = MuonGroup(group_name="my_group_0", detector_IDs=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_IDs=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_IDs=[3])
        self.data.add_group(group1)
        self.data.add_group(group2)
        self.data.add_group(group3)

    def add_two_pairs_to_table(self):
        pair1 = MuonPair(pair_name="my_pair_0", forward_group_name="my_group_0", backward_group_name="my_group_1", alpha=1.0)
        pair2 = MuonPair(pair_name="my_pair_1", forward_group_name="my_group_1", backward_group_name="my_group_2", alpha=1.0)
        self.presenter.add_pair(pair1)
        self.presenter.add_pair(pair2)

    def get_group_1_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 1)

    def get_group_2_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 2)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : test the functionality around alpha.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_alpha_defaults_to_1(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.view.get_table_item_text(0, 3), "1.0")

    def test_that_table_reverts_to_previous_value_when_adding_values_which_arent_numbers_to_alpha_column(self):
        self.presenter.handle_add_pair_button_clicked()

        non_numeric_alphas = ["", "a", "long", "!", "_", "1+2"]

        default_value = self.view.get_table_item_text(0, 3)
        for invalid_alpha in non_numeric_alphas:
            self.view.pairing_table.setCurrentCell(0, 3)
            self.view.pairing_table.item(0, 3).setText(invalid_alpha)

            self.assertEqual(self.view.get_table_item_text(0, 3), default_value)

    def test_that_warning_displayed_when_adding_invalid_alpha_values(self):
        self.presenter.handle_add_pair_button_clicked()

        non_numeric_alphas = ["", "a", "long", "!", "_", "1+2"]

        call_count = 0
        for invalid_alpha in non_numeric_alphas:
            call_count += 1
            self.view.pairing_table.setCurrentCell(0, 3)
            self.view.pairing_table.item(0, 3).setText(invalid_alpha)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

    def test_that_alpha_values_stored_to_three_decimal_places(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 3)
        # test that rounds correctly
        self.view.pairing_table.item(0, 3).setText("1.1239")

        self.assertEqual(self.view.get_table_item_text(0, 3), "1.124")

    def test_that_valid_alpha_values_are_added_correctly(self):
        self.presenter.handle_add_pair_button_clicked()

        valid_inputs = ["1.0", "0.0", "12", ".123", "0.00001", "0.0005"]
        expected_output = ["1.0", "0.0", "12.0", "0.123", "0.0", "0.001"]

        for valid_alpha, expected_alpha in iter(zip(valid_inputs, expected_output)):
            print(valid_alpha, expected_alpha)
            self.view.pairing_table.setCurrentCell(0, 3)
            self.view.pairing_table.item(0, 3).setText(valid_alpha)

            self.assertEqual(self.view.get_table_item_text(0, 3), expected_alpha)

    def test_that_negative_alpha_is_not_allowed(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 3)
        default_value = self.view.get_table_item_text(0, 3)
        self.view.pairing_table.item(0, 3).setText("-1.0")

        self.assertEqual(self.view.get_table_item_text(0, 3), default_value)
        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_clicking_guess_alpha_triggers_correct_slot_with_correct_row_supplied(self):
        # Guess alpha functionality must be implemented by parent widgets. So we just check that the
        # design for implementing this works (via an Observable in the presenter)
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.guessAlphaNotifier.notify_subscribers = mock.Mock()

        self.view.pairing_table.cellWidget(1, 4).clicked.emit(True)

        self.assertEqual(self.presenter.guessAlphaNotifier.notify_subscribers.call_count, 1)
        self.assertEqual(self.presenter.guessAlphaNotifier.notify_subscribers.call_args_list[0][0][0],
                         ["pair_1", "", ""])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
