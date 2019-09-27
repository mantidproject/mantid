# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel

VANADIUM_NUMBER = 307521
CERIUM_NUMBER = 305738
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
class_path = 'Engineering.gui.engineering_diffraction.tabs.calibration.model.CalibrationModel'


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()

    def test_fails_on_invalid_run_number(self):
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "FAIL", 305738, True)
        self.assertRaises(RuntimeError, self.model.create_new_calibration, 307521, "FAIL", True)

    @patch(class_path + '.run_calibration')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    def test_EnggVanadiumCorrections_algorithm_is_called(self, alg, load_ceria, calib):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False)
        alg.assert_called_once()

    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '.run_calibration')
    def test_EnggCalibrate_algorithm_is_called(self, calibrate_alg, vanadium_alg, load_ceria):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False)
        self.assertEqual(calibrate_alg.call_count, 1)

    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '.plot_vanadium_curves')
    @patch(class_path + '._plot_difc_zero')
    @patch(class_path + '.run_calibration')
    def test_plotting_check(self, calib, plot_difc_zero, plot_van, van, ceria):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False)
        plot_van.assert_not_called()
        plot_difc_zero.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True)
        plot_van.assert_called_once()
        self.assertEqual(plot_difc_zero.call_count, 2)


if __name__ == '__main__':
    unittest.main()
