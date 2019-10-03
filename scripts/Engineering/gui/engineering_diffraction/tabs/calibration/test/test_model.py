# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel

VANADIUM_NUMBER = "307521"
CERIUM_NUMBER = "305738"
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
class_path = 'Engineering.gui.engineering_diffraction.tabs.calibration.model.CalibrationModel'


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()

    def test_fails_on_invalid_run_number(self):
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "FAIL", "305738", True,
                          "ENGINX")
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "307521", "FAIL", True,
                          "ENGINX")

    @patch(class_path + '.create_output_files')
    @patch(class_path + '.run_calibration')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    def test_EnggVanadiumCorrections_algorithm_is_called(self, alg, load_ceria, calib,
                                                         output_files):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        alg.assert_called_once()

    @patch(class_path + '.create_output_files')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '.run_calibration')
    def test_EnggCalibrate_algorithm_is_called(self, calibrate_alg, vanadium_alg, load_ceria,
                                               output_files):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(calibrate_alg.call_count, 1)

    @patch(class_path + '.create_output_files')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._plot_difc_zero')
    @patch(class_path + '.run_calibration')
    def test_plotting_check(self, calib, plot_difc_zero, plot_van, van, ceria, output_files):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        plot_van.assert_not_called()
        plot_difc_zero.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX")
        plot_van.assert_called_once()
        self.assertEqual(plot_difc_zero.call_count, 2)

    @patch(class_path + '.create_output_files')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._plot_difc_zero')
    @patch(class_path + '.run_calibration')
    def test_present_RB_number_results_in_user_output_files(self, calib, plot_difc_zero, plot_van,
                                                            van, ceria, output_files):
        self.model.create_new_calibration(VANADIUM_NUMBER,
                                          CERIUM_NUMBER,
                                          False,
                                          "ENGINX",
                                          rb_num="00110")
        self.assertEqual(output_files.call_count, 2)

    @patch(class_path + '.create_output_files')
    @patch(class_path + '.load_ceria')
    @patch(class_path + '.calculate_vanadium_correction')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._plot_difc_zero')
    @patch(class_path + '.run_calibration')
    def test_absent_run_number_results_in_no_user_output_files(self, calib, plot_difc_zero,
                                                               plot_van, van, ceria, output_files):
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(output_files.call_count, 1)

    @patch(class_path + '._generate_output_file_name')
    @patch('Engineering.gui.engineering_diffraction.tabs.calibration.model.makedirs')
    @patch(
        'Engineering.gui.engineering_diffraction.tabs.calibration.model.write_ENGINX_GSAS_iparam_file'
    )
    def test_create_output_files(self, write_file, make_dirs, output_name):
        ceria_run = "20"
        vanadium_run = "10"
        filename = "output"
        output_name.return_value = filename, vanadium_run, ceria_run

        self.model.create_output_files("test/", [0, 0], [1, 1], "test2/test3", "test4/test5",
                                       "ENGINX")

        self.assertEqual(make_dirs.call_count, 1)
        self.assertEqual(write_file.call_count, 3)
        write_file.assert_called_with("test/" + filename, [0], [1],
                                      bank_names=['South'],
                                      ceria_run=ceria_run,
                                      template_file="template_ENGINX_241391_236516_South_bank.prm",
                                      vanadium_run=vanadium_run)

    def test_generate_table_workspace_name(self):
        self.assertEqual(self.model._generate_table_workspace_name(20),
                         "engggui_calibration_bank_21")

    def test_generate_output_file_name_for_valid_bank(self):
        filename, vanadium, ceria = self.model._generate_output_file_name(
            "test/20.raw", "test/10.raw", "ENGINX", "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")
        self.assertEqual(vanadium, '20')
        self.assertEqual(ceria, '10')

        filename, vanadium, ceria = self.model._generate_output_file_name(
            "test/20.raw", "test/10.raw", "ENGINX", "south")
        self.assertEqual(filename, "ENGINX_20_10_bank_South.prm")
        self.assertEqual(vanadium, '20')
        self.assertEqual(ceria, '10')

        filename, vanadium, ceria = self.model._generate_output_file_name(
            "test/20.raw", "test/10.raw", "ENGINX", "all")
        self.assertEqual(filename, "ENGINX_20_10_all_banks.prm")
        self.assertEqual(vanadium, '20')
        self.assertEqual(ceria, '10')

    def test_generate_output_file_name_for_invalid_bank(self):
        self.assertRaises(ValueError, self.model._generate_output_file_name, "test/20.raw",
                          "test/10.raw", "ENGINX", "INVALID")

    def test_generate_output_file_name_for_unconventional_filename(self):
        filename, vanadium, ceria = self.model._generate_output_file_name(
            "test/20", "test/10.raw", "ENGINX", "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")
        self.assertEqual(vanadium, '20')
        self.assertEqual(ceria, '10')

        filename, vanadium, ceria = self.model._generate_output_file_name(
            "20", "test/10.raw", "ENGINX", "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")
        self.assertEqual(vanadium, '20')
        self.assertEqual(ceria, '10')


if __name__ == '__main__':
    unittest.main()
