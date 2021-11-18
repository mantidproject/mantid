# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from unittest.mock import call, patch, create_autospec
from numpy import array
from os import path
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo
from testhelpers import assert_any_call_partial

file_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration.model"


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()
        self.calibration_info = create_autospec(CalibrationInfo())
        mock.NonCallableMock.assert_any_call_partial = assert_any_call_partial

    @patch(file_path + ".path")
    @patch(file_path + ".load_full_instrument_calibration")
    @patch(file_path + ".CalibrationModel.write_diff_consts_to_table_from_prm")
    def test_load_existing_calibration_files_valid_prm(self, mock_write_diff_consts, mock_load_full_calib, mock_path):
        mock_path.exists.return_value = True  # prm file exists

        self.model.load_existing_calibration_files(self.calibration_info)

        mock_load_full_calib.assert_called_once()
        mock_write_diff_consts.assert_called_once_with(self.calibration_info.prm_filepath)
        self.calibration_info.load_relevant_calibration_files.assert_called_once()

    def test_read_diff_constants_from_prm(self):
        file_content = """ID    ENGIN-X CALIBRATION WITH CeO2 and V-Nb
INS    CALIB   241391   ceo2
INS  1 ICONS  18306.98      2.99     14.44
INS  2 ICONS  18497.75    -29.68    -26.50"""
        mocked_handle = mock.mock_open(read_data=file_content)
        dummy_file_path = "/foo/bar_123.prm"
        patchable = "builtins.open"
        with mock.patch(patchable, mocked_handle):
            diff_consts = self.model.read_diff_constants_from_prm(dummy_file_path)
        deltas = abs(diff_consts - array([[2.99, 18306.98, 14.44], [-29.68, 18497.75, -26.5]]))
        self.assertTrue((deltas < 1e-10).all())

    @patch(file_path + ".copy2")
    @patch(file_path + ".makedirs")
    @patch(file_path + ".path.exists")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".CalibrationModel.write_prm_file")
    def test_create_output_files_makes_savdir_and_saves_both_banks(self, mock_write_prm, mock_save_nxs, mock_exists,
                                                                   mock_mkdir, mock_copy):
        mock_exists.return_value = False  # make new directory
        calibration = CalibrationInfo()  # easier to work with real calibration info object here
        prm_name = "ENGINX_193749_all_banks.prm"
        calibration.set_calibration_from_prm_fname(prm_name)
        calibration.set_calibration_table("cal_table")
        save_dir = "savedir"

        self.model.create_output_files(save_dir, calibration, "ws")

        mock_mkdir.assert_called_once_with(save_dir)
        self.calibration_info.save_grouping_workspace.assert_not_called()  # only called if not bank data
        prm_fpath = path.join(save_dir, prm_name)
        write_prm_calls = [call("ws", prm_fpath),
                           call("ws", prm_fpath.replace("all_banks", "bank_1"), spec_nums=[0]),
                           call("ws", prm_fpath.replace("all_banks", "bank_2"), spec_nums=[1])]
        mock_write_prm.assert_has_calls(write_prm_calls)
        nxs_fpath = prm_fpath.replace(".prm", ".nxs")
        mock_save_nxs.assert_called_once_with(InputWorkspace="cal_table", Filename=nxs_fpath)
        copy_calls = [call(nxs_fpath, nxs_fpath.replace("all_banks", "bank_1")),
                      call(nxs_fpath, nxs_fpath.replace("all_banks", "bank_2"))]
        mock_copy.assert_has_calls(copy_calls)

    @patch(file_path + ".copy2")
    @patch(file_path + ".path")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".CalibrationModel.write_prm_file")
    def test_create_output_files_saves_custom_group_file(self, mock_write_prm, mock_save_nxs, mock_path, mock_copy):
        mock_path.exists.return_value = True
        prm_fname = "prm.prm"
        mock_path.join.return_value = prm_fname
        mock_path.splitext.return_value = (prm_fname.replace(".prm", ""), None)
        self.calibration_info.group.banks = None  # no bank data e.g. custom
        self.calibration_info.generate_output_file_name.return_value = prm_fname
        self.calibration_info.get_calibration_table.return_value = "cal_table"  # no bank data e.g. custom
        save_dir = "savedir"

        self.model.create_output_files(save_dir, self.calibration_info, "ws")

        self.calibration_info.save_grouping_workspace.assert_called_once_with(save_dir)
        mock_write_prm.assert_called_once_with("ws", prm_fname)
        mock_save_nxs.assert_called_once_with(InputWorkspace="cal_table", Filename=prm_fname.replace(".prm", ".nxs"))
        mock_copy.assert_not_called()


if __name__ == '__main__':
    unittest.main()
