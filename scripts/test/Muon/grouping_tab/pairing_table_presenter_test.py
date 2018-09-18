import unittest
import sys

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
        pair1 = MuonPair(pair_name="my_pair_0", group1_name="my_group_0", group2_name="my_group_1",alpha=1.0)
        pair2 = MuonPair(pair_name="my_pair_1", group1_name="my_group_1", group2_name="my_group_2",alpha=1.0)
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

    def test_that_highlighting_rows_and_clicking_remove_group_removes_the_selected_rows(self):
        self.assertEqual(0, 0)

    def test_that_cannot_add_more_than_20_rows(self):
        self.assertEqual(0, 0)

    def test_that_trying_to_add_a_20th_row_gives_warning_message(self):
        self.assertEqual(0, 0)

    def test_that_remove_group_when_table_is_empty_does_not_throw(self):
        self.assertEqual(0, 0)

    #
    # Testing context menu??
    #

    #
    # Group name validation
    #

    def test_that_if_invalid_name_given_warning_message_is_shown(self):
        self.assertEqual(0, 0)

    def test_that_if_changing_name_to_invalid_name_value_returns_to_original(self):
        self.assertEqual(0, 0)

    def test_that_group_names_with_numbers_and_letters_and_underscores_are_valid(self):
        self.assertEqual(0, 0)

    def test_that_group_names_with_non_alphanumeric_characters_are_invalid(self):
        self.assertEqual(0, 0)

    def test_that_warning_shown_if_duplicated_group_name_used(self):
        self.assertEqual(0, 0)

    def test_that_default_group_name_is_group_0(self):
        self.assertEqual(0, 0)

    def test_that_adding_new_group_creates_incremented_default_name(self):
        self.assertEqual(0, 0)

    #
    # detector ID validation
    #

    def test_that_if_not_entering_numbers_into_detector_IDs_that_the_code_throws(self):
        self.assertEqual(0, 0)

    def test_that_displayed_values_are_simplified_to_least_verbose_form(self):
        self.assertEqual(0, 0)

    def test_that_if_detector_list_changed_that_number_of_detectors_updates(self):
        self.assertEqual(0, 0)

    def test_that_detector_numbers_cannot_be_edited(self):
        self.assertEqual(0, 0)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
