# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_options_view import (GeneralFittingOptionsView,
                                                                                          SIMULTANEOUS_FIT_LABEL,
                                                                                          SINGLE_FIT_LABEL)

from qtpy.QtWidgets import QApplication


@start_qapplication
class GeneralFittingOptionsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = GeneralFittingOptionsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_the_simultaneous_options_shown_when_it_is_not_a_frequency_domain(self):
        self.view = GeneralFittingOptionsView(is_frequency_domain=False)
        self.view.show()

        self.assertTrue(not self.view.simul_fit_checkbox.isHidden())
        self.assertTrue(not self.view.simul_fit_by_combo.isHidden())
        self.assertTrue(not self.view.simul_fit_by_specifier.isHidden())

    def test_that_the_view_has_been_initialized_with_the_simultaneous_options_hidden_when_it_is_a_frequency_domain(self):
        self.view = GeneralFittingOptionsView(is_frequency_domain=True)
        self.view.show()

        self.assertTrue(self.view.simul_fit_checkbox.isHidden())
        self.assertTrue(self.view.simul_fit_by_combo.isHidden())
        self.assertTrue(self.view.simul_fit_by_specifier.isHidden())

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

    def test_that_number_of_datasets_will_return_the_expected_number_of_datasets(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)

        self.assertEqual(self.view.number_of_datasets(), len(dataset_names))

    def test_that_the_simultaneous_fit_by_can_be_set_as_expected(self):
        self.assertEqual(self.view.simultaneous_fit_by, "Run")

        self.view.simultaneous_fit_by = "Group/Pair"

        self.assertEqual(self.view.simultaneous_fit_by, "Group/Pair")

    def test_that_current_dataset_index_will_return_the_expected_dataset_index(self):
        dataset_names = ["Name1", "Name2", "Name3"]

        self.view.update_dataset_name_combo_box(dataset_names)
        self.view.current_dataset_name = "Name2"

        self.assertEqual(self.view.current_dataset_index, 1)

    def test_that_current_dataset_index_will_return_none_when_there_is_nothing_selected_in_the_combobox(self):
        self.assertEqual(self.view.current_dataset_index, None)

    def test_that_switch_to_simultaneous_will_change_the_relevant_label(self):
        self.view.switch_to_simultaneous()
        self.assertEqual(self.view.workspace_combo_box_label.text(), SIMULTANEOUS_FIT_LABEL)

    def test_that_switch_to_single_will_change_the_relevant_label(self):
        self.view.switch_to_simultaneous()
        self.view.switch_to_single()
        self.assertEqual(self.view.workspace_combo_box_label.text(), SINGLE_FIT_LABEL)

    def test_that_hide_simultaneous_fit_options_will_hide_the_simultaneous_fitting_options(self):
        self.assertTrue(not self.view.simul_fit_checkbox.isHidden())
        self.assertTrue(not self.view.simul_fit_by_combo.isHidden())
        self.assertTrue(not self.view.simul_fit_by_specifier.isHidden())

        self.view.hide_simultaneous_fit_options()

        self.assertTrue(self.view.simul_fit_checkbox.isHidden())
        self.assertTrue(self.view.simul_fit_by_combo.isHidden())
        self.assertTrue(self.view.simul_fit_by_specifier.isHidden())

    def test_that_enable_simultaneous_fit_options_will_hide_the_simultaneous_fitting_options(self):
        self.assertTrue(not self.view.simul_fit_by_combo.isEnabled())
        self.assertTrue(not self.view.simul_fit_by_specifier.isEnabled())

        self.view.enable_simultaneous_fit_options()

        self.assertTrue(self.view.simul_fit_by_combo.isEnabled())
        self.assertTrue(self.view.simul_fit_by_specifier.isEnabled())

    def test_that_disable_simultaneous_fit_options_will_hide_the_simultaneous_fitting_options(self):
        self.view.enable_simultaneous_fit_options()
        self.assertTrue(self.view.simul_fit_by_combo.isEnabled())
        self.assertTrue(self.view.simul_fit_by_specifier.isEnabled())

        self.view.disable_simultaneous_fit_options()

        self.assertTrue(not self.view.simul_fit_by_combo.isEnabled())
        self.assertTrue(not self.view.simul_fit_by_specifier.isEnabled())

    def test_that_is_simultaneous_fit_ticked_will_change_the_fitting_mode_as_expected(self):
        self.assertTrue(not self.view.simul_fit_checkbox.isChecked())

        self.view.simultaneous_fitting_mode = True

        self.assertTrue(self.view.simul_fit_checkbox.isChecked())

    def test_that_setup_fit_by_specifier_will_add_fit_specifiers_to_the_relevant_checkbox(self):
        fit_specifiers = ["long", "fwd", "bwd"]

        self.view.setup_fit_by_specifier(fit_specifiers)

        data = [self.view.simul_fit_by_specifier.itemText(i) for i in range(self.view.simul_fit_by_specifier.count())]
        self.assertTrue(data, fit_specifiers)


if __name__ == '__main__':
    unittest.main()
