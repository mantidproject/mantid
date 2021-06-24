# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.basic_fitting.workspace_selector_view import WorkspaceSelectorView

from qtpy.QtWidgets import QApplication


@start_qapplication
class WorkspaceSelectorViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = WorkspaceSelectorView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_update_dataset_name_combo_box_will_set_the_names_in_the_dataset_name_combobox(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)

        data = [self.view.dataset_name_combo_box.itemText(i) for i in range(self.view.dataset_name_combo_box.count())]
        self.assertTrue(data, dataset_names)

    def test_that_update_dataset_name_combo_box_will_select_the_previously_selected_item_if_it_still_exists(self):
        selected_dataset = "Name3"
        dataset_names = ["Name1", "Name2", selected_dataset]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.dataset_name_combo_box.setCurrentIndex(2)

        new_dataset_names = ["Name4", selected_dataset, "Name5"]
        self.view.update_dataset_name_combo_box(new_dataset_names)

        self.assertTrue(self.view.current_dataset_name, selected_dataset)

    def test_that_increment_dataset_name_combo_box_will_increment_the_dataset_which_is_selected(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.dataset_name_combo_box.setCurrentIndex(2)
        self.assertTrue(self.view.current_dataset_name, "Name3")

        self.view.increment_dataset_name_combo_box()
        self.assertTrue(self.view.current_dataset_name, "Name1")

    def test_that_decrement_dataset_name_combo_box_will_decrement_the_dataset_which_is_selected(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.dataset_name_combo_box.setCurrentIndex(2)
        self.assertTrue(self.view.current_dataset_name, "Name1")

        self.view.decrement_dataset_name_combo_box()
        self.assertTrue(self.view.current_dataset_name, "Name3")

    def test_that_the_current_dataset_name_can_be_set_as_expected(self):
        selected_dataset = "Name2"
        dataset_names = ["Name1", selected_dataset, "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.assertEqual(self.view.current_dataset_name, "Name1")

        self.view.current_dataset_name = selected_dataset
        self.assertEqual(self.view.current_dataset_name, selected_dataset)

    def test_that_the_current_dataset_name_will_not_change_the_selected_dataset_if_the_provided_dataset_does_not_exist(self):
        selected_dataset = "Name3"
        dataset_names = ["Name1", "Name2", selected_dataset]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.current_dataset_name = selected_dataset
        self.assertEqual(self.view.current_dataset_name, selected_dataset)

        self.view.current_dataset_name = "Does not exist"
        self.assertEqual(self.view.current_dataset_name, selected_dataset)

    def test_that_add_dataset_name_will_add_a_name_to_the_end_of_the_datasets(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.assertEqual(self.view.number_of_datasets(), 3)

        self.view.add_dataset_name("Name4")
        self.assertEqual(self.view.number_of_datasets(), 4)

    def test_that_number_of_datasets_will_return_the_expected_number_of_datasets(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)

        self.assertEqual(self.view.number_of_datasets(), len(dataset_names))

    def test_that_current_dataset_index_will_return_the_expected_dataset_index(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.current_dataset_name = "Name2"

        self.assertEqual(self.view.current_dataset_index, 1)

    def test_that_current_dataset_index_will_return_none_when_there_is_nothing_selected_in_the_combobox(self):
        self.assertEqual(self.view.current_dataset_index, None)


if __name__ == '__main__':
    unittest.main()
