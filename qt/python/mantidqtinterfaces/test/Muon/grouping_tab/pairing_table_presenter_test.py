# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from mantidqtinterfaces.Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from mantidqtinterfaces.Muon.GUI.Common.muon_group import MuonGroup
from mantidqtinterfaces.Muon.GUI.Common.muon_pair import MuonPair
from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter
from mantidqtinterfaces.Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from mantidqtinterfaces.Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


def pair_name():
    name = []
    for i in range(21):
        name.append("pair_" + str(i + 1))
    return name


@start_qapplication
class PairingTablePresenterTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.add_three_groups_to_model()

        self.model = GroupingTabModel(context=self.context)
        self.view = PairingTableView(parent=self.obj)
        self.presenter = PairingTablePresenter(self.view, self.model)

        self.view.warning_popup = mock.Mock()
        self.view.enter_pair_name = mock.Mock(side_effect=pair_name())

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.pair_names), 0)
        self.assertEqual(len(self.model.pairs), 0)

    def assert_view_empty(self):
        self.assertEqual(self.presenter.num_rows(), 0)

    def add_three_groups_to_model(self):
        group1 = MuonGroup(group_name="my_group_0", detector_ids=[1])
        group2 = MuonGroup(group_name="my_group_1", detector_ids=[2])
        group3 = MuonGroup(group_name="my_group_2", detector_ids=[3])
        self.group_context.add_group(group1)
        self.group_context.add_group(group2)
        self.group_context.add_group(group3)

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
        self.assertEqual(self.presenter.num_cols(), 6)

    def test_that_model_is_initialized_as_empty(self):
        self.assert_model_empty()

    def test_that_view_is_initialized_as_empty(self):
        self.assert_view_empty()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Adding and removing groups
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_add_pair_button_adds_pair(self):
        self.presenter.handle_add_pair_button_clicked()
        self.assertEqual(self.presenter.num_rows(), 1)
        self.assertEqual(len(self.model.pairs), 1)

    def test_that_remove_pair_button_removes_group(self):
        self.add_two_pairs_to_table()
        self.presenter.handle_remove_pair_button_clicked()
        self.assertEqual(self.presenter.num_rows(), 1)

    def test_that_add_pair_button_adds_pair_to_end_of_table(self):
        self.add_two_pairs_to_table()

        self.presenter.add_pair(MuonPair(pair_name="new", forward_group_name="my_group_0", backward_group_name="my_group_1"))

        self.assertEqual(self.presenter.get_table_item_text(self.presenter.num_rows() - 1, 0), "new")

    def test_that_remove_pair_button_removes_pair_from_end_of_table(self):
        self.add_two_pairs_to_table()

        self.presenter.handle_remove_pair_button_clicked()

        self.assertEqual(self.presenter.get_table_item_text(self.presenter.num_rows() - 1, 0), "my_pair_0")

    def test_that_highlighting_rows_and_clicking_remove_pair_removes_the_selected_rows(self):
        self.add_two_pairs_to_table()
        self.presenter.get_selected_row_indices = mock.Mock(return_value=[0, 1])

        self.presenter.handle_remove_pair_button_clicked()

        self.assert_model_empty()
        self.assert_view_empty()

    def test_that_cannot_add_more_than_20_rows(self):
        for i in range(21):
            self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.presenter.num_rows(), 20)
        self.assertEqual(len(self.model.pairs), 20)

    def test_that_trying_to_add_a_20th_row_gives_warning_message(self):
        for i in range(21):
            self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_remove_group_when_table_is_empty_does_not_throw(self):
        for i in range(3):
            self.presenter.handle_remove_pair_button_clicked()
        self.view.warning_popup.assert_not_called()

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Context menu has "add pair" and "remove pair" functionality
    # ------------------------------------------------------------------------------------------------------------------

    def test_context_menu_add_pairing_with_no_rows_selected_adds_pair_to_end_of_table(self):
        self.presenter._context_menu_event(0)
        self.view.add_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 1)
        self.assertEqual(self.presenter.num_rows(), 1)
        self.assertEqual(self.presenter.get_table_item_text(0, 0), "pair_1")

    def test_context_menu_add_pairing_with_rows_selected_does_not_add_pair(self):
        self.add_two_pairs_to_table()
        self.presenter.get_selected_row_indices = mock.Mock(return_value=[0])

        self.presenter._context_menu_event(0)

        self.assertFalse(self.view.add_pair_action.isEnabled())

    def test_context_menu_remove_pairing_with_no_rows_selected_removes_last_row(self):
        for i in range(3):
            # names : pair_1, pair_2, pair_3
            self.presenter.handle_add_pair_button_clicked()

        self.presenter._context_menu_event(0)
        self.view.remove_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 2)
        self.assertEqual(self.presenter.num_rows(), 2)
        self.assertEqual(self.presenter.get_table_item_text(0, 0), "pair_1")
        self.assertEqual(self.presenter.get_table_item_text(1, 0), "pair_2")

    def test_context_menu_remove_pairing_removes_selected_rows(self):
        for i in range(3):
            # names : pair_0, pair_1, pair_2
            self.presenter.handle_add_pair_button_clicked()
        self.presenter.get_selected_row_indices = mock.Mock(return_value=[0, 2])

        self.presenter._context_menu_event(0)
        self.view.remove_pair_action.triggered.emit(True)

        self.assertEqual(len(self.model.pairs), 1)
        self.assertEqual(self.presenter.num_rows(), 1)
        self.assertEqual(self.presenter.get_table_item_text(0, 0), "pair_2")

    def test_context_menu_remove_pairing_disabled_if_no_pairs_in_table(self):
        self.presenter._context_menu_event(0)

        self.assertFalse(self.view.remove_pair_action.isEnabled())

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : Pair name validation
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_can_change_pair_name_to_valid_name_and_update_view_and_model(self):
        self.add_two_pairs_to_table()
        self.view.get_pairing_table.setCurrentCell(0, 0)
        self.view.get_pairing_table.item(0, 0).setText("new_name")

        self.assertEqual(self.presenter.get_table_item_text(0, 0), "new_name")
        self.assertIn("new_name", self.model.pair_names)

    def test_that_if_invalid_name_given_warning_message_is_shown(self):
        self.add_two_pairs_to_table()

        invalid_names = ["", "@", "name!", "+-"]
        call_count = self.view.warning_popup.call_count
        for invalid_name in invalid_names:
            call_count += 1
            self.view.get_pairing_table.setCurrentCell(0, 0)
            self.view.get_pairing_table.item(0, 0).setText(invalid_name)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

    def test_that_if_invalid_name_given_name_reverts_to_its_previous_value(self):
        self.add_two_pairs_to_table()

        invalid_names = ["", "@", "name!", "+-"]

        for invalid_name in invalid_names:
            self.view.get_pairing_table.setCurrentCell(0, 0)
            self.view.get_pairing_table.item(0, 0).setText(invalid_name)

            self.assertEqual(str(self.presenter.get_table_item_text(0, 0)), "my_pair_0")
            self.assertIn("my_pair_0", self.model.pair_names)

    def test_that_pair_names_with_numbers_and_letters_and_underscores_are_valid(self):
        self.add_two_pairs_to_table()

        valid_names = ["fwd", "fwd_1", "1234", "FWD0001", "_fwd"]

        for valid_name in valid_names:
            self.view.get_pairing_table.setCurrentCell(0, 0)
            self.view.get_pairing_table.item(0, 0).setText(valid_name)

            self.assertEqual(str(self.presenter.get_table_item_text(0, 0)), valid_name)
            self.assertIn(valid_name, self.model.pair_names)

    def test_that_warning_shown_if_duplicated_pair_name_used(self):
        self.add_two_pairs_to_table()

        self.view.enter_pair_name = mock.Mock(return_value="my_group_1")
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_default_pair_name_is_pair_0(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(str(self.presenter.get_table_item_text(0, 0)), "pair_1")
        self.assertIn("pair_1", self.model.pair_names)

    def test_that_adding_new_pair_creates_incremented_default_name(self):
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(str(self.presenter.get_table_item_text(0, 0)), "pair_1")
        self.assertEqual(str(self.presenter.get_table_item_text(1, 0)), "pair_2")
        self.assertEqual(str(self.presenter.get_table_item_text(2, 0)), "pair_3")
        self.assertCountEqual(self.model.pair_names, ["pair_1", "pair_2", "pair_3"])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
