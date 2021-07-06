# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FrameworkManager
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.basic_fitting.basic_fitting_view import BasicFittingView
from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_function_options_view import RAW_DATA_TABLE_ROW

from qtpy.QtWidgets import QApplication


@start_qapplication
class BasicFittingViewTest(unittest.TestCase, QtWidgetFinder):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.view = BasicFittingView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_plot_guess_checkbox_can_be_ticked_as_expected(self):
        self.view.plot_guess = True
        self.assertTrue(self.view.plot_guess)

    def test_that_the_undo_fit_button_can_be_enabled_as_expected_when_the_number_of_undos_is_above_zero(self):
        self.view.enable_view()
        self.view.set_number_of_undos(2)
        self.assertTrue(self.view.fit_controls.undo_fit_button.isEnabled())

    def test_that_update_global_fit_status_label_will_display_no_fit_if_the_success_list_is_empty(self):
        success_list = []

        self.view.update_global_fit_status(success_list)

        self.assertEqual(self.view.fit_controls.global_fit_status_label.text(), "No Fit")

    def test_that_update_global_fit_status_label_will_display_fit_successful_if_all_fits_are_successful(self):
        success_list = ["success"] * 5

        self.view.update_global_fit_status(success_list)

        self.assertEqual(self.view.fit_controls.global_fit_status_label.text(), "Fit Successful")

    def test_that_update_global_fit_status_label_will_display_fits_failed_if_some_of_the_fits_fail(self):
        success_list = ["success", "success", "fail", "success", "fail"]

        self.view.update_global_fit_status(success_list)

        self.assertEqual(self.view.fit_controls.global_fit_status_label.text(), "2 of 5 fits failed")

    def test_that_the_view_has_been_initialized_with_the_raw_data_option_shown_when_it_is_not_a_frequency_domain(self):
        self.view = BasicFittingView()
        self.view.show()

        self.assertTrue(not self.view.fit_function_options.fit_options_table.isRowHidden(RAW_DATA_TABLE_ROW))

    def test_that_the_view_has_been_initialized_with_the_raw_data_option_hidden_when_it_is_a_frequency_domain(self):
        self.view = BasicFittingView()
        self.view.hide_fit_raw_checkbox()
        self.view.show()

        self.assertTrue(self.view.fit_function_options.fit_options_table.isRowHidden(RAW_DATA_TABLE_ROW))

    def test_that_update_fit_status_labels_will_display_no_fit_if_the_success_list_is_empty(self):
        fit_status, chi_squared = "success", 1.1

        self.view.update_local_fit_status_and_chi_squared(fit_status, chi_squared)

        self.assertEqual(self.view.fit_function_options.fit_status_success_failure.text(), "Success")
        self.assertEqual(self.view.fit_function_options.fit_status_chi_squared.text(), "Chi squared: 1.1")

    def test_that_update_fit_status_labels_will_display_fit_successful_if_all_fits_are_successful(self):
        fit_status, chi_squared = None, 0.0

        self.view.update_local_fit_status_and_chi_squared(fit_status, chi_squared)

        self.assertEqual(self.view.fit_function_options.fit_status_success_failure.text(), "No Fit")
        self.assertEqual(self.view.fit_function_options.fit_status_chi_squared.text(), "Chi squared: 0.0")

    def test_that_update_fit_status_labels_will_display_fits_failed_if_some_of_the_fits_fail(self):
        fit_status, chi_squared = "failed for some reason", 2.2

        self.view.update_local_fit_status_and_chi_squared(fit_status, chi_squared)

        self.assertEqual(self.view.fit_function_options.fit_status_success_failure.text(), f"Failure: {fit_status}")
        self.assertEqual(self.view.fit_function_options.fit_status_chi_squared.text(), "Chi squared: 2.2")

    def test_that_set_datasets_in_function_browser_will_set_the_datasets_in_the_function_browser(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.set_datasets_in_function_browser(dataset_names)

        self.assertEqual(self.view.fit_function_options.number_of_datasets(), 3)

    def test_that_set_current_dataset_index_will_set_the_current_dataset_index_in_the_function_browser(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.set_datasets_in_function_browser(dataset_names)
        self.view.set_current_dataset_index(2)

        self.assertEqual(self.view.fit_function_options.function_browser.getCurrentDataset(), 2)

    def test_that_update_function_browser_parameters_will_update_the_parameters_of_the_function_for_single_fit(self):
        old_function = "name=FlatBackground,A0=0"

        self.view.fit_function_options.function_browser.setFunction(old_function)
        self.assertEqual(str(self.view.fit_object), old_function)

        updated_function = self.view.fit_object.setParameter("A0", 1.0)

        self.view.update_fit_function(updated_function)
        self.assertEqual(str(self.view.fit_object), str(updated_function))

    def test_that_update_function_browser_parameters_will_clear_the_function_if_the_function_provided_is_none(self):
        old_function = "name=FlatBackground,A0=0"

        self.view.fit_function_options.function_browser.setFunction(old_function)
        self.assertEqual(str(self.view.fit_object), old_function)

        self.view.update_fit_function(None)
        self.assertEqual(self.view.fit_object, None)

    def test_that_it_is_possible_to_set_the_start_x_to_a_different_value(self):
        new_value = 5.0

        self.view.start_x = new_value

        self.assertEqual(self.view.start_x, new_value)

    def test_that_it_is_possible_to_set_the_end_x_to_a_different_value(self):
        new_value = 5.0

        self.view.end_x = new_value

        self.assertEqual(self.view.end_x, new_value)

    def test_that_the_fit_to_raw_checkbox_value_can_be_changed_as_expected(self):
        self.view.fit_to_raw = False
        self.assertTrue(not self.view.fit_to_raw)

        self.view.fit_to_raw = True
        self.assertTrue(self.view.fit_to_raw)

    def test_that_the_function_name_can_be_changed_as_expected(self):
        new_function_name = "Test Function Name"

        self.view.function_name = new_function_name

        self.assertEqual(self.view.function_name, new_function_name)


if __name__ == '__main__':
    unittest.main()
