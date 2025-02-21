# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from Engineering.EnggUtils import GROUP
from Engineering.common.calibration_info import CalibrationInfo
from mantid.api import AnalysisDataService as ADS
import numpy as np
from unittest.mock import patch

eng_common = "Engineering.common.calibration_info"

# define a mixin, as there is a lot of reused code to test each group


class SetCalibrationMixin(object):
    def setup_set_calibration_from_prm(self, test_file, group):
        calibration = CalibrationInfo()
        calibration.set_calibration_from_prm_fname(test_file)

        # check the instrument and ceria path are correctly set
        self.assertEqual(calibration.get_instrument(), "ENGINX")
        self.assertEqual(calibration.get_ceria_path(), "123456")
        self.assertEqual(calibration.group, group)

    def setup_get_group_ws(self, group):
        calibration = CalibrationInfo()
        calibration.group = group
        calibration.get_group_ws()

    def setup_load_relevant_calibration_files(self, group, suffix):
        calibration = CalibrationInfo()
        calibration.group = group
        calibration.prm_filepath = f"ENGINX_123456_{suffix}.prm"

        calibration.load_relevant_calibration_files()


class TestCalibrationInfo(unittest.TestCase, SetCalibrationMixin):
    def test_set_calibration_from_prm_group_both(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_all_banks.prm", GROUP.BOTH)

    def test_set_calibration_from_prm_group_bank1(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_bank_1.prm", GROUP.NORTH)

    def test_set_calibration_from_prm_group_bank2(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_bank_2.prm", GROUP.SOUTH)

    def test_set_calibration_from_prm_group_texture20(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_Texture20.prm", GROUP.TEXTURE20)

    def test_set_calibration_from_prm_group_texture30(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_Texture30.prm", GROUP.TEXTURE30)

    def test_set_calibration_from_prm_group_custom(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_Custom.prm", GROUP.CUSTOM)

    def test_set_calibration_from_prm_group_cropped(self):
        self.setup_set_calibration_from_prm("ENGINX_123456_Cropped.prm", GROUP.CROPPED)

    # test that get_group_ws will call the correct method depending on grouping
    # the groupings which call create_bank_grouping_workspace and create_grouping_workspace_from_calfile require files
    # to be loaded which will weigh down unit tests - can presume these loading functions are tested elsewhere

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_get_group_ws_both(self, mock_create_bank_grouping_workspace):
        self.setup_get_group_ws(GROUP.BOTH)

        mock_create_bank_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_get_group_ws_bank1(self, mock_create_bank_grouping_workspace):
        self.setup_get_group_ws(GROUP.NORTH)

        mock_create_bank_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_get_group_ws_bank2(self, mock_create_bank_grouping_workspace):
        self.setup_get_group_ws(GROUP.SOUTH)

        mock_create_bank_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_get_group_ws_texture20(self, mock_create_bank_grouping_workspace):
        self.setup_get_group_ws(GROUP.TEXTURE20)

        mock_create_bank_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_get_group_ws_texture30(self, mock_create_bank_grouping_workspace):
        self.setup_get_group_ws(GROUP.TEXTURE30)

        mock_create_bank_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "create_grouping_workspace_from_calfile")
    def test_get_group_ws_custom(self, mock_create_grouping_workspace_from_calfile):
        self.setup_get_group_ws(GROUP.CUSTOM)

        mock_create_grouping_workspace_from_calfile.assert_called_once()

    def test_get_group_ws_cropped(self):
        # This grouping doesn't require an input file, so we don't need to mock the response
        calibration = CalibrationInfo(instrument="ENGINX")
        calibration.group = GROUP.CROPPED
        calibration.set_spectra_list("1-1200")
        calibration.get_group_ws()

        reference_grouping = np.zeros((2500, 1))
        reference_grouping[:1200] = 1.0

        test_grouping = calibration.group_ws.extractY()

        # check the detector groupings are all the same
        self.assertTrue(np.all(test_grouping == reference_grouping))
        ADS.clear()

    # test that load_relevant_calibration_files will call the correct method depending on grouping

    @patch.object(CalibrationInfo, "get_group_ws")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_both(self, mock_get_group_ws, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.BOTH, "banks")

        mock_Load.assert_called_once()
        mock_get_group_ws.assert_called_once()

    @patch.object(CalibrationInfo, "get_group_ws")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_bank1(self, mock_get_group_ws, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.NORTH, "bank_1")

        mock_Load.assert_called_once()
        mock_get_group_ws.assert_called_once()

    @patch.object(CalibrationInfo, "get_group_ws")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_bank2(self, mock_get_group_ws, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.SOUTH, "bank_2")

        mock_Load.assert_called_once()
        mock_get_group_ws.assert_called_once()

    @patch.object(CalibrationInfo, "get_group_ws")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_texture20(self, mock_get_group_ws, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.TEXTURE20, "Texture20")

        mock_Load.assert_called_once()
        mock_get_group_ws.assert_called_once()

    @patch.object(CalibrationInfo, "get_group_ws")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_texture30(self, mock_get_group_ws, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.TEXTURE30, "Texture30")

        mock_Load.assert_called_once()
        mock_get_group_ws.assert_called_once()

    @patch.object(CalibrationInfo, "load_custom_grouping_workspace")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_custom(self, mock_load_custom_grouping_workspace, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.CUSTOM, "Custom")

        mock_Load.assert_called_once()
        mock_load_custom_grouping_workspace.assert_called_once()

    @patch.object(CalibrationInfo, "load_custom_grouping_workspace")
    @patch(eng_common + ".Load")
    def test_load_relevant_calibration_files_cropped(self, mock_load_custom_grouping_workspace, mock_Load):
        self.setup_load_relevant_calibration_files(GROUP.CROPPED, "Cropped")

        mock_Load.assert_called_once()
        mock_load_custom_grouping_workspace.assert_called_once()


if __name__ == "__main__":
    unittest.main()
