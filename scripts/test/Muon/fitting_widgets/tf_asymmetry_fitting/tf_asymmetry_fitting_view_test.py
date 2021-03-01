# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView

from qtpy.QtWidgets import QApplication


@start_qapplication
class TFAsymmetryFittingViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = TFAsymmetryFittingView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_the_tf_asymmetry_mode_turned_off_and_the_relevant_widgets_hidden(self):
        self.assertTrue(not self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_label.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_tf_asymmetry_mode_can_be_turned_on_and_the_relevant_widgets_shown(self):
        self.view.tf_asymmetry_mode = True

        self.assertTrue(self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_fitting_options.normalisation_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_tf_asymmetry_mode_can_be_turned_off_and_the_relevant_widgets_hidden(self):
        self.view.tf_asymmetry_mode = False

        self.assertTrue(not self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_label.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_the_normalisation_can_be_set_as_expected(self):
        normalisation = 5.0

        self.view.normalisation = normalisation

        self.assertEqual(self.view.normalisation, normalisation)


if __name__ == '__main__':
    unittest.main()
