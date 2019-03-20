import unittest
import sys
from PyQt4 import QtGui

from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_model import PairingTableModel
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_view import PairingTableView
from Muon.GUI.Common.pairing_table_widget.pairing_table_widget_presenter import PairingTablePresenter

from Muon.GUI.Common.muon_group import MuonGroup
from Muon.GUI.Common.muon_pair import MuonPair
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common import mock_widget

if sys.version_info.major > 2:
    from unittest import mock
else:
    import mock

def pair_name():
    name = []
    for i in range(21):
        name.append("pair_" + str(i+1))
    return name

class GroupSelectorTest(unittest.TestCase):

    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        # Store an empty widget to parent all the views, and ensure they are deleted correctly
        self.obj = QtGui.QWidget()

        self.data = MuonDataContext()

        self.model = PairingTableModel(data=self.data)
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
        self.data.add_group(group1)
        self.data.add_group(group2)
        self.data.add_group(group3)

    def add_two_pairs_to_table(self):
        pair1 = MuonPair(pair_name="my_pair_0", forward_group_name="my_group_0", backward_group_name="my_group_1", alpha=1.0)
        pair2 = MuonPair(pair_name="my_pair_1", forward_group_name="my_group_1", backward_group_name="my_group_2", alpha=1.0)
        self.presenter.add_pair(pair1)
        self.presenter.add_pair(pair2)

    def get_group_1_selector_from_pair(self, row):
        return self.view.pairing_table.cellWidget(row, 1)

    def get_group_2_selector_from_pair(self, row):
        return self.view.pairing_table.cellWidget(row, 2)

    # ------------------------------------------------------------------------------------------------------------------
    # TESTS : test the functionality around the combo boxes which allow the user to select the two groups that
    #         together make the muon pair.
    # ------------------------------------------------------------------------------------------------------------------

    def test_that_adding_two_groups_and_then_pair_sets_combo_to_added_groups(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), "my_group_0")
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), "my_group_1")

    def test_that_added_groups_appear_in_group_combo_boxes_in_new_pairs(self):
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 3)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_1"), -1)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_2"), -1)

    def test_that_get_index_of_text_returns_correct_index_if_text_exists(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        index = self.view.get_index_of_text(self.get_group_1_selector_from_pair(0), 'my_group_1')

        self.assertEqual(index, 1)

    def test_that_get_index_of_text_returns_0_if_text_does_not_exists(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        index = self.view.get_index_of_text(self.get_group_1_selector_from_pair(0), 'random string')

        self.assertEqual(index, 0)

    def test_that_added_groups_appear_in_group_combo_boxes_in_existing_pairs_if_update_called(self):
        self.presenter.handle_add_pair_button_clicked()
        self.add_three_groups_to_model()
        # the following method must be called
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 3)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_1"), -1)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_2"), -1)

    def test_that_changing_group_selection_triggers_cell_changed_method_in_view(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()
        self.presenter.handle_add_pair_button_clicked()

        self.view.on_cell_changed = mock.Mock()
        self.get_group_1_selector_from_pair(0).setCurrentIndex(1)

        self.assertEqual(self.view.on_cell_changed.call_count, 1)
        self.assertEqual(self.view.on_cell_changed.call_args_list[0][0], (0, 1))

    def test_that_removing_groups_and_then_calling_update_removes_groups_from_selections(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()
        del self.data.groups["my_group_1"]
        del self.data.groups["my_group_2"]
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 1)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 1)
        self.assertNotEqual(self.get_group_1_selector_from_pair(0).findText("my_group_0"), -1)
        self.assertNotEqual(self.get_group_2_selector_from_pair(0).findText("my_group_0"), -1)

    def test_adding_new_group_does_not_change_pair_selection(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), 'my_group_0')
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), 'my_group_1')

        group4 = MuonGroup(group_name="my_group_4", detector_ids=[4])
        self.data.add_group(group4)
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 4)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 4)
        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), 'my_group_0')
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), 'my_group_1')

    def test_removing_group_used_in_pair_handled_gracefully(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        del self.data.groups["my_group_0"]
        self.presenter.update_view_from_model()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 2)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 2)
        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), 'my_group_1')
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), 'my_group_1')

    def test_group_changed_to_other_group_switches_groups(self):
        self.add_three_groups_to_model()
        self.presenter.handle_add_pair_button_clicked()

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), 'my_group_0')
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), 'my_group_1')

        group_selector = self.get_group_1_selector_from_pair(0)
        group_selector.setCurrentIndex(1)

        self.assertEqual(self.get_group_1_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_2_selector_from_pair(0).count(), 3)
        self.assertEqual(self.get_group_1_selector_from_pair(0).currentText(), 'my_group_1')
        self.assertEqual(self.get_group_2_selector_from_pair(0).currentText(), 'my_group_0')




if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
