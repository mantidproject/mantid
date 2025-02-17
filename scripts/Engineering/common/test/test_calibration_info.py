# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
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

    def setup_get_group_ws(self, group, filepath=None):
        calibration = CalibrationInfo()
        calibration.group = group
        calibration.grouping_filepath = filepath
        calibration.get_group_ws()

    def setup_load_relevant_calibration_files(self, group, suffix):
        calibration = CalibrationInfo()
        calibration.group = group
        calibration.prm_filepath = f"ENGINX_123456_{suffix}.prm"

        calibration.load_relevant_calibration_files()


class TestCalibrationInfo(unittest.TestCase, SetCalibrationMixin):
    def test_set_calibration_from_prm(self):
        test_cases = [
            ("ENGINX_123456_all_banks.prm", GROUP.BOTH),
            ("ENGINX_123456_bank_1.prm", GROUP.NORTH),
            ("ENGINX_123456_bank_2.prm", GROUP.SOUTH),
            ("ENGINX_123456_Texture20.prm", GROUP.TEXTURE20),
            ("ENGINX_123456_Texture30.prm", GROUP.TEXTURE30),
            ("ENGINX_123456_Custom.prm", GROUP.CUSTOM),
            ("ENGINX_123456_Cropped_1-1200.prm", GROUP.CROPPED),
        ]

        for filename, group in test_cases:
            with self.subTest(filename=filename, group=group):
                self.setup_set_calibration_from_prm(filename, group)

    # test that get_group_ws will call the correct method depending on grouping
    # the groupings which call create_bank_grouping_workspace and create_grouping_workspace_from_calfile require files
    # to be loaded which will weigh down unit tests - can presume these loading functions are tested elsewhere

    @patch.object(CalibrationInfo, "create_bank_grouping_workspace")
    def test_setup_get_group_ws(self, mock_create_bank_grouping_workspace):
        test_cases = [GROUP.BOTH, GROUP.NORTH, GROUP.SOUTH, GROUP.TEXTURE20, GROUP.TEXTURE30]

        for group in test_cases:
            with self.subTest(group=group):
                self.setup_get_group_ws(group)
                mock_create_bank_grouping_workspace.assert_called_once()
                mock_create_bank_grouping_workspace.reset_mock()

    @patch.object(CalibrationInfo, "create_grouping_workspace_from_calfile")
    def test_get_group_ws_custom_cal(self, mock_create_grouping_workspace_from_calfile):
        self.setup_get_group_ws(GROUP.CUSTOM, "3x5_group.cal")

        mock_create_grouping_workspace_from_calfile.assert_called_once()
        mock_create_grouping_workspace_from_calfile.reset_mock()

    @patch(eng_common + ".LoadDetectorsGroupingFile")
    def test_get_group_ws_custom_xml(self, mock_LoadDetectorsGroupingFile):
        self.setup_get_group_ws(GROUP.CUSTOM, "3x5_group.xml")

        mock_LoadDetectorsGroupingFile.assert_called_once()
        mock_LoadDetectorsGroupingFile.reset_mock()

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
    def test_setup_load_relevant_calibration_files_get_group_ws(self, mock_load, mock_get_group_ws):
        test_cases = [
            (GROUP.BOTH, "banks"),
            (GROUP.NORTH, "bank_1"),
            (GROUP.SOUTH, "bank_2"),
            (GROUP.TEXTURE20, "Texture20"),
            (GROUP.TEXTURE30, "Texture30"),
        ]

        for group, suffix in test_cases:
            with self.subTest(group=group, suffix=suffix):
                self.setup_load_relevant_calibration_files(group, suffix)

                mock_load.assert_called_once()
                mock_get_group_ws.assert_called_once()

                mock_load.reset_mock(), mock_get_group_ws.reset_mock()

    @patch.object(CalibrationInfo, "load_custom_grouping_workspace")
    @patch(eng_common + ".Load")
    def test_setup_load_relevant_calibration_files_custom_grouping(self, mock_load, mock_load_custom_grouping_workspace):
        test_cases = [
            (GROUP.CUSTOM, "Custom"),
            (GROUP.CROPPED, "Cropped"),
        ]

        for group, suffix in test_cases:
            with self.subTest(group=group, suffix=suffix):
                self.setup_load_relevant_calibration_files(group, suffix)

                mock_load.assert_called_once()
                mock_load_custom_grouping_workspace.assert_called_once()

                mock_load.reset_mock(), mock_load_custom_grouping_workspace.reset_mock()

    def test_set_extra_group_suffix(self):
        test_cases = [("test.xml", "_test"), (None, "")]

        for fp, target in test_cases:
            with self.subTest(fp=fp, target=target):
                calibration = CalibrationInfo()
                calibration.group = GROUP.CUSTOM
                calibration.grouping_filepath = fp

                calibration.set_extra_group_suffix()

                self.assertEqual(calibration.extra_group_suffix, target)

    def test_get_group_suffix(self):
        test_cases = [
            (GROUP.CUSTOM, "test.xml", "Custom_test"),
            (GROUP.CUSTOM, None, "Custom"),  # if no grouping fp can be found still should work
            (GROUP.BOTH, None, "all_banks"),
            (GROUP.BOTH, "NorthAndSouthBanks.xml", "all_banks"),
        ]

        for group, fp, target in test_cases:
            with self.subTest(group=group, fp=fp, target=target):
                calibration = CalibrationInfo()
                calibration.group = group
                calibration.grouping_filepath = fp

                self.assertEqual(calibration.get_group_suffix(), target)

    def test_get_group_suffix_cropped(self):
        test_cases = [
            (GROUP.CROPPED, "1-100", "Cropped_1-100"),
            (GROUP.CROPPED, None, "Cropped"),  # if no grouping fp can be found still should work
        ]

        for group, spec_list_str, target in test_cases:
            with self.subTest(group=group, spec_list_str=spec_list_str, target=target):
                calibration = CalibrationInfo()
                calibration.group = group
                calibration.set_spectra_list(spec_list_str)

                self.assertEqual(calibration.get_group_suffix(), target)


if __name__ == "__main__":
    unittest.main()
