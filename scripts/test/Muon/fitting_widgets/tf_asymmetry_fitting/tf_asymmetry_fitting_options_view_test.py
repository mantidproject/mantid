# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.qt.testing.qt_widget_finder import QtWidgetFinder

from Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_options_view import \
    TFAsymmetryFittingOptionsView

from qtpy.QtWidgets import QApplication


@start_qapplication
class TFAsymmetryFittingOptionsViewTest(unittest.TestCase, QtWidgetFinder):

    def setUp(self):
        self.view = TFAsymmetryFittingOptionsView()
        self.view.show()
        self.assert_widget_created()

    def tearDown(self):
        self.assertTrue(self.view.close())
        QApplication.sendPostedEvents()

    def test_that_the_view_has_been_initialized_with_a_normalisation_line_edit_which_has_a_validator(self):
        self.assertTrue(isinstance(self.view.normalisation_line_edit.validator(), LineEditDoubleValidator))

    def test_that_set_tf_asymmetry_mode_will_show_the_tf_asymmetry_options_when_set_to_true(self):
        self.view.set_tf_asymmetry_mode(True)

        self.assertTrue(not self.view.normalisation_label.isHidden())
        self.assertTrue(not self.view.normalisation_line_edit.isHidden())

    def test_that_set_tf_asymmetry_mode_will_hide_the_tf_asymmetry_options_when_set_to_false(self):
        self.view.set_tf_asymmetry_mode(True)
        self.view.set_tf_asymmetry_mode(False)

        self.assertTrue(self.view.normalisation_label.isHidden())
        self.assertTrue(self.view.normalisation_line_edit.isHidden())

    def test_that_the_normalisation_can_be_set_as_expected(self):
        normalisation = 5.0

        self.view.normalisation = normalisation

        self.assertEqual(self.view.normalisation, normalisation)

    def test_that_show_normalisation_options_will_show_all_of_the_tf_asymmetry_options(self):
        self.view.show_normalisation_options()

        self.assertTrue(not self.view.normalisation_label.isHidden())
        self.assertTrue(not self.view.normalisation_line_edit.isHidden())

    def test_that_hide_normalisation_options_will_hide_all_of_the_tf_asymmetry_options(self):
        self.view.show_normalisation_options()
        self.view.hide_normalisation_options()

        self.assertTrue(self.view.normalisation_label.isHidden())
        self.assertTrue(self.view.normalisation_line_edit.isHidden())


if __name__ == '__main__':
    unittest.main()
