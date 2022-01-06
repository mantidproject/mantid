# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import call, patch, create_autospec
from os import path
from Engineering.EnggUtils import GROUP
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel
from Engineering.common.calibration_info import CalibrationInfo
from testhelpers import assert_any_call_partial

file_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model"
enggutils_path = "Engineering.EnggUtils"


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()
        self.calibration_info = create_autospec(CalibrationInfo())
        mock.NonCallableMock.assert_any_call_partial = assert_any_call_partial

    @patch(enggutils_path + ".path")
    @patch(file_path + ".load_full_instrument_calibration")
    @patch(enggutils_path + ".write_diff_consts_to_table_from_prm")
    def test_load_existing_calibration_files_valid_prm(self, mock_write_diff_consts, mock_load_full_calib, mock_path):
        mock_path.exists.return_value = True  # prm file exists

        self.model.load_existing_calibration_files(self.calibration_info)

        mock_load_full_calib.assert_called_once()
        mock_write_diff_consts.assert_called_once_with(self.calibration_info.prm_filepath)
        self.calibration_info.load_relevant_calibration_files.assert_called_once()

    @patch(enggutils_path + '.mantid.DeleteWorkspace')
    @patch(enggutils_path + '.create_output_files')
    @patch(enggutils_path + '.make_diff_consts_table')
    @patch(enggutils_path + '.run_calibration')
    @patch(file_path + ".load_full_instrument_calibration")
    @patch(enggutils_path + ".path_handling.load_workspace")
    def test_non_texture_output_files_saved_to_two_directories_when_rb_num(self, mock_load_ws, mock_load_cal,
                                                                           mock_run_cal, mock_make_diff_table,
                                                                           mock_create_out, mock_del):
        rb_num = "1"
        self.calibration_info.group = GROUP.BOTH
        mock_run_cal.return_value = ("foc_ceria_ws", None, None)  # focused_ceria, cal_table, diag_ws

        self.model.create_new_calibration(self.calibration_info, rb_num, plot_output=False, save_dir='dir')

        create_out_calls = [call(path.join('dir', 'Calibration',''), self.calibration_info, 'foc_ceria_ws'),
                            call(path.join('dir', 'User', '1', 'Calibration', ''),
                                 self.calibration_info, 'foc_ceria_ws')]
        self.assertEqual(mock_create_out.call_count, 2)
        mock_create_out.assert_has_calls(create_out_calls)

    @patch(enggutils_path + '.mantid.DeleteWorkspace')
    @patch(enggutils_path + '.create_output_files')
    @patch(enggutils_path + '.make_diff_consts_table')
    @patch(enggutils_path + '.run_calibration')
    @patch(file_path + ".load_full_instrument_calibration")
    @patch(enggutils_path + ".path_handling.load_workspace")
    def test_texture_output_files_saved_to_one_directories_when_rb_num(self, mock_load_ws, mock_load_cal,
                                                                       mock_run_cal, mock_make_diff_table,
                                                                       mock_create_out, mock_del):
        rb_num = "1"
        self.calibration_info.group = GROUP.TEXTURE20
        mock_run_cal.return_value = ("foc_ceria_ws", None, None)  # focused_ceria, cal_table, diag_ws

        self.model.create_new_calibration(self.calibration_info, rb_num, plot_output=False, save_dir='dir')

        mock_create_out.assert_called_once_with(path.join('dir', 'User', '1', 'Calibration', ''),
                                                self.calibration_info, 'foc_ceria_ws')

    @patch(enggutils_path + '.mantid.DeleteWorkspace')
    @patch(enggutils_path + '.create_output_files')
    @patch(enggutils_path + '.make_diff_consts_table')
    @patch(enggutils_path + '.run_calibration')
    @patch(file_path + ".load_full_instrument_calibration")
    @patch(enggutils_path + ".path_handling.load_workspace")
    def test_texture30_output_files_saved_to_one_directories_when_rb_num(self, mock_load_ws, mock_load_cal,
                                                                         mock_run_cal, mock_make_diff_table,
                                                                         mock_create_out, mock_del):
        rb_num = "1"
        self.calibration_info.group = GROUP.TEXTURE30
        mock_run_cal.return_value = ("foc_ceria_ws", None, None)  # focused_ceria, cal_table, diag_ws

        self.model.create_new_calibration(self.calibration_info, rb_num, plot_output=False, save_dir='dir')

        mock_create_out.assert_called_once_with(path.join('dir', 'User', '1', 'Calibration', ''),
                                                self.calibration_info, 'foc_ceria_ws')


if __name__ == '__main__':
    unittest.main()
