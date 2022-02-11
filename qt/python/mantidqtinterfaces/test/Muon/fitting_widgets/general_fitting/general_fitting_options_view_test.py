# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.general_fitting.general_fitting_options_view import GeneralFittingOptionsView


@start_qapplication
class GeneralFittingOptionsViewTest(unittest.TestCase):

    def setUp(self):
        self.view = GeneralFittingOptionsView()
        self.view.show()

    def tearDown(self):
        self.assertTrue(self.view.close())

    def test_that_the_view_can_be_initialized_without_an_error(self):
        self.view = GeneralFittingOptionsView()
        self.view.show()

    def test_that_the_simultaneous_fit_by_can_be_set_as_expected(self):
        self.assertEqual(self.view.simultaneous_fit_by, "Run")

        self.view.simultaneous_fit_by = "Group/Pair"

        self.assertEqual(self.view.simultaneous_fit_by, "Group/Pair")

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
