# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.list_selector.list_selector_presenter import ListSelectorPresenter


@start_qapplication
class TestListSelectorPresenter(unittest.TestCase):
    def setUp(self):
        self.view = mock.MagicMock()
        self.model = {"property_one": [2, True, True], "property_two": [1, False, False]}
        self.presenter = ListSelectorPresenter(self.view, self.model)

    def test_presenter_initialised_with_correct_variables(self):
        self.assertEqual(self.presenter.model, self.model)
        self.assertEqual(self.presenter.view, self.view)

    def test_get_filtered_list_returns_correctly_filtered_list_in_correct_order(self):
        self.presenter.filter_string = "one"
        self.model.update({"test_one": [0, False, False]})

        filtered_list = self.presenter.get_filtered_list()

        self.assertEqual(filtered_list, [["test_one", False, False], ["property_one", True, True]])

    def test_handle_filter_changed_creates_filtered_model_and_updates_view_accordingly(self):
        self.presenter.handle_filter_changed("one")

        self.view.addItems.assert_called_once_with([["property_one", True, True]])

    def test_handle_filter_changed_creates_filtered_model_and_updates_view_accordingly_for_no_filter(self):
        self.presenter.handle_filter_changed("")

        self.view.addItems.assert_called_once_with([["property_two", False, False], ["property_one", True, True]])

    def test_handle_selction_changed_updates_model_accordingly(self):
        self.presenter.handle_selection_changed("property_two", True)

        self.assertEqual(self.model["property_two"][1], True)

    def test_get_selected_items_returns_items_names_of_selected_items(self):
        self.assertEqual(self.presenter.get_selected_items(), ["property_one"])

    def test_get_selected_items_and_positions_returns_names_of_selected_items(self):
        self.assertEqual(self.presenter.get_selected_items_and_positions(), [("property_one", 2)])

    def test_set_filter_type_changes_filter_type_and_updated_view(self):
        self.view.filter_type_combo_box.currentText.return_value = "Exclude"
        self.presenter.set_filter_type()
        self.presenter.handle_filter_changed("one")

        self.assertEqual(self.presenter.filter_type, "Exclude")

        self.view.addItems.assert_called_with([["property_two", False, False]])

    def test_select_all_presenter_checkbox_changed_updates_current_filter_list_to_match_itself(self):
        self.presenter.handle_select_all_checkbox_changed(True)

        self.view.addItems.assert_called_with([["property_two", True, False], ["property_one", True, True]])

    def test_that_adding_to_the_filter_list_applies_additional_filter_on_items(self):
        self.presenter.update_filter_list(["property_two"])

        self.view.addItems.assert_called_with([["property_one", True, True]])


if __name__ == "__main__":
    unittest.main(buffer=False, verbosity=2)
