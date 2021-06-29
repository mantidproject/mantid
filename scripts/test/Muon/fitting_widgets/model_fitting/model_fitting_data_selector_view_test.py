# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.model_fitting.model_fitting_data_selector_view import ModelFittingDataSelectorView

from qtpy.QtWidgets import QApplication


@start_qapplication
class ModelFittingDataSelectorViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = ModelFittingDataSelectorView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_update_result_table_names_will_set_the_names_in_the_result_table_names_combobox(self):
        result_table_names = ["Name1", "Name2", "Name3"]

        self.view.update_result_table_names(result_table_names)

        data = [self.view.result_table_selector.dataset_name_combo_box.itemText(i)
                for i in range(self.view.result_table_selector.dataset_name_combo_box.count())]
        self.assertTrue(data, result_table_names)

    def test_that_update_result_table_names_will_select_the_previously_selected_item_if_it_still_exists(self):
        selected_dataset = "Name3"
        dataset_names = ["Name1", "Name2", selected_dataset]

        self.view.update_result_table_names(dataset_names)
        self.view.result_table_selector.dataset_name_combo_box.setCurrentIndex(2)

        new_dataset_names = ["Name4", selected_dataset, "Name5"]
        self.view.update_result_table_names(new_dataset_names)

        self.assertTrue(self.view.result_table_selector.current_dataset_name, selected_dataset)

    def test_that_add_results_table_name_will_add_a_result_table_to_the_workspace_selector(self):
        result_table_names = ["Name1", "Name2", "Name3"]

        self.view.update_result_table_names(result_table_names)
        self.assertEqual(self.view.number_of_result_tables(), 3)

        self.view.add_results_table_name("Name4")
        self.assertEqual(self.view.number_of_result_tables(), 4)

    def test_that_update_x_and_y_parameters_will_update_the_x_and_y_parameters(self):
        x_parameters = ["workspace_name", "A0", "A1"]
        y_parameters = ["workspace_name", "A0", "A1", "Chi Squared"]

        self.view.update_x_parameters(x_parameters)
        self.view.update_y_parameters(y_parameters)

        x_data = [self.view.x_selector.itemText(i) for i in range(self.view.x_selector.count())]
        y_data = [self.view.y_selector.itemText(i) for i in range(self.view.y_selector.count())]

        self.assertTrue(x_data, x_parameters)
        self.assertTrue(y_data, y_parameters)


if __name__ == '__main__':
    unittest.main()
