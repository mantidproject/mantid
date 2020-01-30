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
file_path = 'Engineering.gui.engineering_diffraction.tabs.calibration.model'


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()

    def test_fails_on_invalid_run_number(self):
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "FAIL", "305738", True,
                          "ENGINX")
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "307521", "FAIL", True,
                          "ENGINX")

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(class_path + '.run_calibration')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    def test_EnggVanadiumCorrections_algorithm_is_called(self, van, load_sample, calib,
                                                         output_files, update_table):
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        van.assert_called_once()

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(class_path + '.run_calibration')
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    def test_fetch_vanadium_is_called(self, van_corr, calibrate_alg, load_sample, output_files,
                                      update_table):
        van_corr.return_value = ("mocked_integration", "mocked_curves")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(van_corr.call_count, 1)

    @patch(file_path + '.path.exists')
    @patch(file_path + '.get_setting')
    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(class_path + '.run_calibration')
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    def test_having_full_calib_set_uses_file(self, van_corr, calibrate_alg, load_workspace, output_files,
                                             update_table, setting, path):
        path.return_value = True
        setting.return_value = "mocked/out/path"
        van_corr.return_value = ("mocked_integration", "mocked_curves")
        load_workspace.return_value = "mocked_workspace"
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        calibrate_alg.assert_called_with("mocked_workspace", "mocked_integration", "mocked_curves",
                                         full_calib_ws="mocked_workspace")

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._generate_difc_tzero_workspace')
    @patch(class_path + '._plot_difc_tzero')
    @patch(class_path + '.run_calibration')
    def test_plotting_check(self, calib, plot_difc_zero, gen_difc, plot_van, van, sample,
                            output_files, update_table):
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        plot_van.assert_not_called()
        plot_difc_zero.assert_not_called()
        gen_difc.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX")
        plot_van.assert_called_once()
        self.assertEqual(gen_difc.call_count, 2)
        self.assertEqual(plot_difc_zero.call_count, 1)

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._plot_difc_tzero')
    @patch(class_path + '.run_calibration')
    def test_present_RB_number_results_in_user_output_files(self, calib, plot_difc_zero, plot_van,
                                                            van, sample, output_files,
                                                            update_table):
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER,
                                          CERIUM_NUMBER,
                                          False,
                                          "ENGINX",
                                          rb_num="00110")
        self.assertEqual(output_files.call_count, 2)

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    @patch(class_path + '._plot_vanadium_curves')
    @patch(class_path + '._plot_difc_tzero')
    @patch(class_path + '.run_calibration')
    def test_absent_run_number_results_in_no_user_output_files(self, calib, plot_difc_zero,
                                                               plot_van, van, sample, output_files,
                                                               update_table):
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(output_files.call_count, 1)

    @patch(class_path + '.update_calibration_params_table')
    @patch(class_path + '.create_output_files')
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + '.vanadium_corrections.fetch_correction_workspaces')
    @patch(class_path + '.run_calibration')
    def test_calibration_params_table_is_updated(self, calibrate_alg, vanadium_alg, load_sample,
                                                 output_files, update_table):
        vanadium_alg.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(calibrate_alg.call_count, 1)

    @patch(class_path + '._generate_output_file_name')
    @patch('Engineering.gui.engineering_diffraction.tabs.calibration.model.makedirs')
    @patch(
        'Engineering.gui.engineering_diffraction.tabs.calibration.model.write_ENGINX_GSAS_iparam_file'
    )
    def test_create_output_files(self, write_file, make_dirs, output_name):
        sample_path = "test2/test3/ENGINX20.nxs"
        vanadium_path = "test4/ENGINX0010.nxs"
        filename = "output"
        output_name.return_value = filename

        self.model.create_output_files("test/", [0, 0], [1, 1], sample_path, vanadium_path,
                                       "ENGINX")

        self.assertEqual(make_dirs.call_count, 1)
        self.assertEqual(write_file.call_count, 3)
        write_file.assert_called_with("test/" + filename, [0], [1],
                                      bank_names=['South'],
                                      ceria_run=sample_path,
                                      template_file="template_ENGINX_241391_236516_South_bank.prm",
                                      vanadium_run=vanadium_path)

    def test_generate_table_workspace_name(self):
        self.assertEqual(self.model._generate_table_workspace_name(20),
                         "engggui_calibration_bank_21")

    def test_generate_output_file_name_for_north_bank(self):
        filename = self.model._generate_output_file_name("test/20.raw", "test/10.raw", "ENGINX",
                                                         "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")

    def test_generate_output_file_name_for_south_bank(self):
        filename = self.model._generate_output_file_name("test/20.raw", "test/10.raw", "ENGINX",
                                                         "south")
        self.assertEqual(filename, "ENGINX_20_10_bank_South.prm")

    def test_generate_output_file_name_for_both_banks(self):
        filename = self.model._generate_output_file_name("test/20.raw", "test/10.raw", "ENGINX",
                                                         "all")
        self.assertEqual(filename, "ENGINX_20_10_all_banks.prm")

    def test_generate_output_file_name_for_invalid_bank(self):
        self.assertRaises(ValueError, self.model._generate_output_file_name, "test/20.raw",
                          "test/10.raw", "ENGINX", "INVALID")

    def test_generate_output_file_name_with_no_ext_in_filename(self):
        filename = self.model._generate_output_file_name("test/20", "test/10.raw", "ENGINX",
                                                         "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")

    def test_generate_output_file_name_with_no_path_in_filename(self):
        filename = self.model._generate_output_file_name("20.raw", "test/10.raw", "ENGINX", "north")
        self.assertEqual(filename, "ENGINX_20_10_bank_North.prm")

    @patch("Engineering.gui.engineering_diffraction.tabs.calibration.model.Ads")
    def test_update_calibration_params_table_retrieves_workspace(self, ads):
        table = [[0, 18414.900000000001, 0.0, -11.82], [1, 18497.75, 0.0, -26.5]]

        self.model.update_calibration_params_table(table)

        self.assertEqual(ads.retrieve.call_count, 1)

    @patch("Engineering.gui.engineering_diffraction.tabs.calibration.model.Ads")
    def test_update_calibration_params_table_stops_when_table_empty(self, ads):
        table = []

        self.model.update_calibration_params_table(table)

        self.assertEqual(ads.retrieve.call_count, 0)


if __name__ == '__main__':
    unittest.main()
