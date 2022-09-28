# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_fitting_view import TFAsymmetryFittingView


@start_qapplication
class TFAsymmetryFittingViewTest(unittest.TestCase):

    def setUp(self):
        self.view = TFAsymmetryFittingView()
        self.view.show()

    def tearDown(self):
        self.assertTrue(self.view.close())

    def test_that_the_view_has_been_initialized_with_the_tf_asymmetry_mode_turned_off_and_the_relevant_widgets_hidden(self):
        self.assertTrue(not self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.fix_normalisation_checkbox.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_tf_asymmetry_mode_can_be_turned_on_and_the_relevant_widgets_shown(self):
        self.view.tf_asymmetry_mode = True

        self.assertTrue(self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_fitting_options.fix_normalisation_checkbox.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_tf_asymmetry_mode_can_be_turned_off_and_the_relevant_widgets_hidden(self):
        self.view.tf_asymmetry_mode = False

        self.assertTrue(not self.view.tf_asymmetry_mode)
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_label.isHidden())
        self.assertTrue(not self.view.tf_asymmetry_mode_switcher.fitting_type_combo_box.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.fix_normalisation_checkbox.isHidden())
        self.assertTrue(self.view.tf_asymmetry_fitting_options.normalisation_line_edit.isHidden())

    def test_that_the_normalisation_can_be_set_as_expected(self):
        normalisation, error = 5.0, 0.5

        self.view.set_normalisation(normalisation, error)

        self.assertEqual(self.view.normalisation, normalisation)
        self.assertEqual(self.view.tf_asymmetry_fitting_options.normalisation_error_line_edit.text(), f"({error:.5f})")

    def test_that_fix_normalisation_is_turned_off_by_default(self):
        self.assertTrue(not self.view.is_normalisation_fixed)

    def test_that_is_normalisation_fixed_can_be_set_to_true(self):
        self.view.is_normalisation_fixed = True
        self.assertTrue(self.view.is_normalisation_fixed)


if __name__ == '__main__':
    unittest.main()
