import unittest
from PyQt4 import QtGui

from mantid.py3compat import mock

from Muon.GUI.Common import mock_widget
from Muon.GUI.Common.contexts.muon_context import MuonContext
from Muon.GUI.Common.contexts.muon_data_context import MuonDataContext
from Muon.GUI.Common.contexts.muon_group_pair_context import MuonGroupPairContext
from Muon.GUI.Common.contexts.muon_gui_context import MuonGuiContext
from Muon.GUI.Common.grouping_tab_widget.grouping_tab_widget_model import GroupingTabModel
from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_load_data import MuonLoadData
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView


def pair_name():
    name = []
    for i in range(21):
        name.append("pair_" + str(i+1))
    return name

class AlphaTest(unittest.TestCase):

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.loaded_data = MuonLoadData()
        self.data_context = MuonDataContext(self.loaded_data)
        self.gui_context = MuonGuiContext()
        self.group_context = MuonGroupPairContext()
        self.context = MuonContext(muon_data_context=self.data_context, muon_group_context=self.group_context,
                                   muon_gui_context=self.gui_context)

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

    def test_that_alpha_values_stored_to_three_decimal_places_when_rounding_down(self):
        self.presenter.handle_add_pair_button_clicked()

        self.view.pairing_table.setCurrentCell(0, 3)
        # test that rounds correctly
        self.view.pairing_table.item(0, 3).setText("1.1244")

        self.assertEqual(self.view.get_table_item_text(0, 3), "1.124")

    def test_that_valid_alpha_values_are_added_correctly(self):
        self.presenter.handle_add_pair_button_clicked()

        valid_inputs = ["1.0", "12", ".123", "0.00001", "0.0005"]
        expected_output = ["1.0", "12.0", "0.123", "1e-05", "0.001"]

        for valid_alpha, expected_alpha in iter(zip(valid_inputs, expected_output)):
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
                         ["pair_2", "my_group_0", "my_group_1"])


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
