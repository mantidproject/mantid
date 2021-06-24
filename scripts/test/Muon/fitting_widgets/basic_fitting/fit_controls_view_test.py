# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.basic_fitting.fit_controls_view import FitControlsView

from qtpy.QtWidgets import QApplication


@start_qapplication
class FitControlsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = FitControlsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_the_undo_fit_button_disabled_and_plot_guess_unchecked(self):
        self.assertTrue(not self.view.undo_fit_button.isEnabled())
        self.assertTrue(not self.view.plot_guess)

    def test_that_the_plot_guess_checkbox_can_be_ticked_as_expected(self):
        self.view.plot_guess = True
        self.assertTrue(self.view.plot_guess)

    def test_that_the_undo_fit_button_can_be_enabled_when_you_set_more_than_zero_undos(self):
        self.view.set_number_of_undos(2)
        self.assertTrue(self.view.undo_fit_button.isEnabled())

    def test_that_the_undo_fit_button_can_be_enabled_when_you_set_one_undos(self):
        self.view.set_number_of_undos(1)
        self.assertTrue(self.view.undo_fit_button.isEnabled())

    def test_that_the_undo_fit_button_can_be_disabled_when_you_set_zero_undos(self):
        self.view.set_number_of_undos(2)
        self.assertTrue(self.view.undo_fit_button.isEnabled())

        self.view.set_number_of_undos(0)
        self.assertTrue(not self.view.undo_fit_button.isEnabled())

    def test_that_update_global_fit_status_label_will_display_no_fit_if_the_success_list_is_empty(self):
        success_list = []

        self.view.update_global_fit_status_label(success_list)

        self.assertEqual(self.view.global_fit_status_label.text(), "No Fit")

    def test_that_update_global_fit_status_label_will_display_fit_successful_if_all_fits_are_successful(self):
        success_list = [True] * 5

        self.view.update_global_fit_status_label(success_list)

        self.assertEqual(self.view.global_fit_status_label.text(), "Fit Successful")

    def test_that_update_global_fit_status_label_will_display_fits_failed_if_some_of_the_fits_fail(self):
        success_list = [True, True, False, True, False]

        self.view.update_global_fit_status_label(success_list)

        self.assertEqual(self.view.global_fit_status_label.text(), "2 of 5 fits failed")


if __name__ == '__main__':
    unittest.main()
