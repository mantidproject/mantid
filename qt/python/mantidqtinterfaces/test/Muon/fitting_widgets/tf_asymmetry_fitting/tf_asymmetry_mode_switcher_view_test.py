# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantidqt.utils.qt.testing import start_qapplication

from mantidqtinterfaces.Muon.GUI.Common.fitting_widgets.tf_asymmetry_fitting.tf_asymmetry_mode_switcher_view import \
    TFAsymmetryModeSwitcherView


@start_qapplication
class TFAsymmetryModeSwitcherViewTest(unittest.TestCase):

    def setUp(self):
        self.view = TFAsymmetryModeSwitcherView()
        self.view.show()

    def tearDown(self):
        self.assertTrue(self.view.close())

    def test_that_the_tf_asymmetry_mode_can_be_set_to_be_on(self):
        self.view.tf_asymmetry_mode = False
        self.view.tf_asymmetry_mode = True

        self.assertTrue(self.view.tf_asymmetry_mode)

    def test_that_the_tf_asymmetry_mode_can_be_set_to_be_off(self):
        self.view.tf_asymmetry_mode = True
        self.view.tf_asymmetry_mode = False

        self.assertTrue(not self.view.tf_asymmetry_mode)

    def test_that_hide_fitting_type_combobox_will_hide_the_fitting_type_combobox(self):
        self.assertTrue(not self.view.fitting_type_label.isHidden())
        self.assertTrue(not self.view.fitting_type_combo_box.isHidden())

        self.view.hide_fitting_type_combobox()

        self.assertTrue(self.view.fitting_type_label.isHidden())
        self.assertTrue(self.view.fitting_type_combo_box.isHidden())


if __name__ == '__main__':
    unittest.main()
