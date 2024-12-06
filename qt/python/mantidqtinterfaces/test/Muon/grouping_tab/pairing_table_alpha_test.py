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
class AlphaTest(unittest.TestCase):
    def setUp(self):
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QWidget()

        setup_context_for_tests(self)

        self.model = GroupingTabModel(context=self.context)
        self.view = PairingTableView(parent=self.obj)
        self.presenter = PairingTablePresenter(self.view, self.model)

        self.add_three_groups_to_model()

        self.view.warning_popup = mock.Mock()
        self.view.enter_pair_name = mock.Mock(side_effect=pair_name())

    def tearDown(self):
        self.obj = None

    def assert_model_empty(self):
        self.assertEqual(len(self.model.pair_names), 0)
        self.assertEqual(len(self.model.pairs), 0)

    def assert_view_empty(self):
        self.assertEqual(self.view.num_rows(), 0)

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

    def get_group_1_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 1)

    def get_group_2_selector(self, row):
        return self.view.pairing_table.cellWidget(row, 2)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : test the functionality around alpha.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_alpha_defaults_to_1(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.presenter.get_table_item_text(0, 4), "1.0")

    def test_that_table_reverts_to_previous_value_when_adding_values_which_arent_numbers_to_alpha_column(self):
        self.presenter.handle_add_pair_button_clicked()

        non_numeric_alphas = ["", "a", "long", "!", "_", "1+2"]

        default_value = self.presenter.get_table_item_text(0, 4)
        for invalid_alpha in non_numeric_alphas:
            self.view.pairing_table.setCurrentCell(0, 4)
            self.view.pairing_table.item(0, 4).setText(invalid_alpha)

            self.assertEqual(self.presenter.get_table_item_text(0, 4), default_value)

    def test_that_warning_displayed_when_adding_invalid_alpha_values(self):
        self.presenter.handle_add_pair_button_clicked()

        non_numeric_alphas = ["", "a", "long", "!", "_", "1+2"]

        call_count = 0
        for invalid_alpha in non_numeric_alphas:
            call_count += 1
            self.view.pairing_table.setCurrentCell(0, 4)
            self.view.pairing_table.item(0, 4).setText(invalid_alpha)

            self.assertEqual(self.view.warning_popup.call_count, call_count)

    def test_that_alpha_values_stored_to_correct_decimal_places(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 4)
        # test that rounds correctly
        self.view.pairing_table.item(0, 4).setText("1.1234567890")

        self.assertEqual(self.presenter.get_table_item_text(0, 4), "1.123457")

    def test_that_alpha_values_stored_to_correct_decimal_places_when_rounding_down(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 4)
        # test that rounds correctly
        self.view.pairing_table.item(0, 4).setText("1.12345617890")

        self.assertEqual(self.presenter.get_table_item_text(0, 4), "1.123456")

    def test_that_valid_alpha_values_are_added_correctly(self):
        self.presenter.handle_add_pair_button_clicked()

        valid_inputs = ["1.0", "12", ".123", "0.0000011", "0.05e-6"]
        expected_output = [1.0, 12.0, 0.123, 1e-6, 1e-6]

        for valid_alpha, expected_alpha in iter(zip(valid_inputs, expected_output)):
            self.view.pairing_table.setCurrentCell(0, 4)
            self.view.pairing_table.item(0, 4).setText(valid_alpha)
            # make presenter update
            self.presenter.handle_data_change(0, 4)
            self.assertEqual(float(self.presenter.get_table_item_text(0, 4)), expected_alpha)

    def test_that_negative_alpha_is_not_allowed(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 4)
        default_value = self.presenter.get_table_item_text(0, 4)
        self.view.pairing_table.item(0, 4).setText("-1.0")

        self.assertEqual(self.presenter.get_table_item_text(0, 4), default_value)
        self.assertEqual(self.view.warning_popup.call_count, 1)

    def test_that_clicking_guess_alpha_triggers_correct_slot_with_correct_row_supplied(self):
        # Guess alpha functionality must be implemented by parent widgets. So we just check that the
        # design for implementing this works (via an Observable in the presenter)
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.guessAlphaNotifier.notify_subscribers = mock.Mock()

        self.view.pairing_table.cellWidget(1, 5).clicked.emit(True)

        self.assertEqual(self.presenter.guessAlphaNotifier.notify_subscribers.call_count, 1)
        self.assertEqual(
            self.presenter.guessAlphaNotifier.notify_subscribers.call_args_list[0][0][0], ["pair_2", "my_group_0", "my_group_1"]
        )


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
