# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.line_edit_double_validator import LineEditDoubleValidator
from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_options_view import \
    TFAsymmetryFittingOptionsView


@start_qapplication
class TFAsymmetryFittingOptionsViewTest(unittest.TestCase):

    def setUp(self):
        self.view = TFAsymmetryFittingOptionsView()
        self.view.show()

    def tearDown(self):
        self.assertTrue(self.view.close())

    def test_that_the_view_has_been_initialized_with_a_normalisation_line_edit_which_has_a_validator(self):
        self.assertTrue(isinstance(self.view.normalisation_line_edit.validator(), LineEditDoubleValidator))

    def test_that_set_tf_asymmetry_mode_will_show_the_tf_asymmetry_options_when_set_to_true(self):
        self.view.set_tf_asymmetry_mode(True)

        self.assertTrue(not self.view.fix_normalisation_checkbox.isHidden())
        self.assertTrue(not self.view.normalisation_line_edit.isHidden())
        self.assertTrue(not self.view.normalisation_error_line_edit.isHidden())

    def test_that_set_tf_asymmetry_mode_will_hide_the_tf_asymmetry_options_when_set_to_false(self):
        self.view.set_tf_asymmetry_mode(True)
        self.view.set_tf_asymmetry_mode(False)

        self.assertTrue(self.view.fix_normalisation_checkbox.isHidden())
        self.assertTrue(self.view.normalisation_line_edit.isHidden())
        self.assertTrue(self.view.normalisation_error_line_edit.isHidden())

    def test_that_the_normalisation_can_be_set_as_expected(self):
        normalisation, error = 5.0, 0.5

        self.view.set_normalisation(normalisation, error)

        self.assertEqual(self.view.normalisation, normalisation)
        self.assertEqual(self.view.normalisation_error_line_edit.text(), f"({error:.5f})")

    def test_that_show_normalisation_options_will_show_all_of_the_tf_asymmetry_options(self):
        self.view.show_normalisation_options()

        self.assertTrue(not self.view.fix_normalisation_checkbox.isHidden())
        self.assertTrue(not self.view.normalisation_line_edit.isHidden())
        self.assertTrue(not self.view.normalisation_error_line_edit.isHidden())

    def test_that_hide_normalisation_options_will_hide_all_of_the_tf_asymmetry_options(self):
        self.view.show_normalisation_options()
        self.view.hide_normalisation_options()

        self.assertTrue(self.view.fix_normalisation_checkbox.isHidden())
        self.assertTrue(self.view.normalisation_line_edit.isHidden())
        self.assertTrue(self.view.normalisation_error_line_edit.isHidden())

    def test_that_fix_normalisation_is_turned_off_by_default(self):
        self.assertTrue(not self.view.is_normalisation_fixed)

    def test_that_is_normalisation_fixed_can_be_set_to_true(self):
        self.view.is_normalisation_fixed = True
        self.assertTrue(self.view.is_normalisation_fixed)


if __name__ == '__main__':
    unittest.main()
