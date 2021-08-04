# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import FrameworkManager, FunctionFactory
from mantid.simpleapi import CreateEmptyTableWorkspace
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_function_options_view import (FitFunctionOptionsView,
                                                                                     RAW_DATA_TABLE_ROW)
from Muon.GUI.Common.utilities.workspace_utils import StaticWorkspaceWrapper

from qtpy.QtWidgets import QApplication


@start_qapplication
class FitFunctionOptionsViewTest(unittest.TestCase, QtWidgetFinder):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def setUp(self):
        self.view = FitFunctionOptionsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_the_raw_data_option_shown(self):
        self.view = FitFunctionOptionsView()
        self.view.show()

        self.assertTrue(not self.view.fit_options_table.isRowHidden(RAW_DATA_TABLE_ROW))

    def test_that_the_view_has_been_initialized_with_the_raw_data_option_hidden(self):
        self.view = FitFunctionOptionsView()
        self.view.hide_fit_raw_checkbox()
        self.view.show()

        self.assertTrue(self.view.fit_options_table.isRowHidden(RAW_DATA_TABLE_ROW))

    def test_that_update_fit_status_labels_will_display_no_fit_if_the_success_list_is_empty(self):
        fit_status, chi_squared = "success", 1.1

        self.view.update_fit_status_labels(fit_status, chi_squared)

        self.assertEqual(self.view.fit_status_success_failure.text(), "Success")
        self.assertEqual(self.view.fit_status_chi_squared.text(), "Chi squared: 1.1")

    def test_that_update_fit_status_labels_will_display_fit_successful_if_all_fits_are_successful(self):
        fit_status, chi_squared = None, 0.0

        self.view.update_fit_status_labels(fit_status, chi_squared)

        self.assertEqual(self.view.fit_status_success_failure.text(), "No Fit")
        self.assertEqual(self.view.fit_status_chi_squared.text(), "Chi squared: 0")

    def test_that_update_fit_status_labels_will_display_fits_failed_if_some_of_the_fits_fail(self):
        fit_status, chi_squared = "failed for some reason", 2.2

        self.view.update_fit_status_labels(fit_status, chi_squared)

        self.assertEqual(self.view.fit_status_success_failure.text(), f"Failure: {fit_status}")
        self.assertEqual(self.view.fit_status_chi_squared.text(), "Chi squared: 2.2")

    def test_that_clear_fit_status_will_clear_the_fit_status_and_chi_squared(self):
        fit_status, chi_squared = "failed for some reason", 2.2

        self.view.update_fit_status_labels(fit_status, chi_squared)
        self.view.clear_fit_status()

        self.assertEqual(self.view.fit_status_success_failure.text(), "No Fit")
        self.assertEqual(self.view.fit_status_chi_squared.text(), "Chi squared: 0.0")

    def test_that_set_datasets_in_function_browser_will_set_the_datasets_in_the_function_browser(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.set_datasets_in_function_browser(dataset_names)

        self.assertEqual(self.view.number_of_datasets(), 3)

    def test_that_set_current_dataset_index_will_set_the_current_dataset_index_in_the_function_browser(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.set_datasets_in_function_browser(dataset_names)
        self.view.set_current_dataset_index(2)

        self.assertEqual(self.view.function_browser.getCurrentDataset(), 2)

    def test_that_update_function_browser_parameters_will_update_the_parameters_of_the_function_for_single_fit(self):
        old_function = "name=FlatBackground,A0=0"
        simultaneous_mode = False

        self.view.function_browser.setFunction(old_function)
        self.assertEqual(str(self.view.fit_object), old_function)

        updated_function = self.view.fit_object.setParameter("A0", 1.0)

        self.view.update_function_browser_parameters(simultaneous_mode, updated_function)
        self.assertEqual(str(self.view.fit_object), str(updated_function))

    def test_that_update_function_browser_parameters_will_set_the_function_if_in_simultaneous_mode(self):
        old_function = "name=FlatBackground,A0=0"
        simultaneous_mode = True

        self.view.function_browser.setFunction(old_function)
        self.assertEqual(str(self.view.fit_object), old_function)

        updated_function = self.view.fit_object.setParameter("A0", 1.0)

        self.view.update_function_browser_parameters(simultaneous_mode, updated_function)
        self.assertEqual(str(self.view.fit_object), str(updated_function))

    def test_that_update_function_browser_parameters_will_clear_the_function_if_the_function_provided_is_none(self):
        old_function = "name=FlatBackground,A0=0"
        simultaneous_mode = False

        self.view.function_browser.setFunction(old_function)
        self.assertEqual(str(self.view.fit_object), old_function)

        self.view.update_function_browser_parameters(simultaneous_mode, None)
        self.assertEqual(self.view.fit_object, None)

    def test_that_set_fit_function_will_set_the_function_in_the_browser(self):
        fit_function = FunctionFactory.createFunction("FlatBackground")

        self.view.set_fit_function(fit_function)
        self.assertEqual(str(self.view.current_fit_function()), str(fit_function))

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

    def test_that_set_covariance_button_enabled_can_disable_the_covariance_button(self):
        self.assertTrue(self.view.covariance_matrix_button.isEnabled())
        self.view.set_covariance_button_enabled(False)
        self.assertTrue(not self.view.covariance_matrix_button.isEnabled())

    def test_that_set_covariance_button_enabled_can_enable_the_covariance_button(self):
        self.view.set_covariance_button_enabled(False)
        self.view.set_covariance_button_enabled(True)
        self.assertTrue(self.view.covariance_matrix_button.isEnabled())

    def test_that_show_normalised_covariance_matrix_will_not_raise_an_error(self):
        ws = CreateEmptyTableWorkspace()
        wrapper = StaticWorkspaceWrapper("CovarianceMatrix", ws)

        self.view.show_normalised_covariance_matrix(wrapper.workspace, wrapper.workspace_name)


if __name__ == '__main__':
    unittest.main()
