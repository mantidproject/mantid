# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from unittest.mock import patch, MagicMock

from Engineering.gui.engineering_diffraction.tabs.calibration.model import CalibrationModel
from testhelpers import assert_any_call_partial

VANADIUM_NUMBER = "307521"
CERIUM_NUMBER = "305738"
TEST_PRM_FILE = "ENGINX_241391_236516_testall_banks.prm"
class_path = "Engineering.gui.engineering_diffraction.tabs.calibration.model.CalibrationModel"
file_path = "Engineering.gui.engineering_diffraction.tabs.calibration.model"


def _convert_units_returns(bank):
    if bank == '1':
        return "engggui_calibration_bank_1"
    elif bank == '2':
        return "engggui_calibration_bank_2"
    elif bank == 'cropped':
        return "engggui_calibration_Cropped"


def _run_calibration_returns(single_output=False):
    calib = [{"difa": 0, "difc": 0, "tzero": 0}, {"difa": 0, "difc": 0, "tzero": 0}]
    calib_single = [{"difa": 0, "difc": 0, "tzero": 0}]
    sample = MagicMock()
    grp_ws = MagicMock()
    if single_output:
        return calib_single, sample, grp_ws
    return calib, sample, grp_ws


class CalibrationModelTest(unittest.TestCase):
    def setUp(self):
        self.model = CalibrationModel()
        mock.NonCallableMock.assert_any_call_partial = assert_any_call_partial

    def test_fails_on_invalid_run_number(self):
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "FAIL", "305738", True,
                          "ENGINX")
        self.assertRaises(RuntimeError, self.model.create_new_calibration, "307521", "FAIL", True,
                          "ENGINX")

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".Load")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(class_path + ".run_calibration")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_fetch_vanadium_is_called(self, van_corr, calibrate_alg, load_sample, load, output_files,
                                      update_table, delet):
        van_corr.return_value = ("mocked_integration", "mocked_curves")
        calibrate_alg.return_value = _run_calibration_returns()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(van_corr.call_count, 1)

    @patch(class_path + ".get_info_from_file")
    @patch(file_path + ".path")
    @patch(file_path + ".EnggUtils")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(class_path + ".update_calibration_params_table")
    def test_load_existing_calibration_files(self, update_table, fetch_ws, utils, path, get_info):
        path.exists.return_value = True
        get_info.return_value = "TESTINST", "van_no", "ceria_no", [["params"], ["table"]]
        utils.load_custom_grouping_workspace.return_value = "grp_ws", "Region"
        inst, van_no, ceria_no, grp_ws, roi_text, bank = self.model.load_existing_calibration_files(TEST_PRM_FILE)

        table_test_params = [["params"], ["table"]]
        update_table.assert_called_with(table_test_params)
        self.assertEqual(1, update_table.call_count)

        fetch_ws.assert_called_with("TESTINSTvan_no", "TESTINST", is_load=True)
        self.assertEqual(1, fetch_ws.call_count)

        self.assertEqual(1, utils.load_relevant_calibration_files.call_count)

        self.assertEqual("TESTINST", inst)
        self.assertEqual("van_no", van_no)
        self.assertEqual("ceria_no", ceria_no)
        self.assertEqual("grp_ws", grp_ws)
        self.assertEqual("Region", roi_text)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".Load")
    @patch(file_path + ".EnggUtils")
    @patch(class_path + ".run_calibration")
    def test_plotting_check(self, calib, utils, load, van, sample, output_files, update_table,
                            delete):
        calib.return_value = _run_calibration_returns()
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        utils.plot_tof_fit.assert_not_called()
        utils.generate_tof_fit_dictionary.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX")
        self.assertEqual(utils.plot_tof_fit.call_count, 1)
        self.assertEqual(utils.generate_tof_fit_dictionary.call_count, 2)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".Load")
    @patch(file_path + ".EnggUtils")
    @patch(class_path + ".run_calibration")
    def test_plotting_check_cropped(self, calib, utils, load, van, sample,
                                    output_files, update_table, delete):
        calib.return_value = _run_calibration_returns(single_output=True)
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        utils.plot_tof_fit.assert_not_called()
        utils.generate_tof_fit_dictionary.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX", bank=1)
        self.assertEqual(utils.plot_tof_fit.call_count, 1)
        self.assertEqual(utils.generate_tof_fit_dictionary.call_count, 1)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".Load")
    @patch(class_path + ".run_calibration")
    def test_present_RB_number_results_in_user_output_files(self, calib, load, van, sample,
                                                            output_files, update_table, delete):
        van.return_value = ("A", "B")
        calib.return_value = _run_calibration_returns()
        self.model.create_new_calibration(VANADIUM_NUMBER,
                                          CERIUM_NUMBER,
                                          False,
                                          "ENGINX",
                                          rb_num="00110")
        self.assertEqual(output_files.call_count, 2)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".Load")
    @patch(class_path + ".run_calibration")
    def test_absent_run_number_results_in_no_user_output_files(self, calib, load,
                                                               van, sample, output_files, update_table, delete):
        calib.return_value = _run_calibration_returns()
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(output_files.call_count, 1)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(class_path + ".run_calibration")
    def test_calibration_params_table_is_updated(self, calibrate_alg, vanadium_alg, load, load_sample,
                                                 output_files, update_table, delete):
        calibrate_alg.return_value = _run_calibration_returns()
        vanadium_alg.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(calibrate_alg.call_count, 1)

    def test_generate_table_workspace_name(self):
        self.assertEqual(self.model._generate_table_workspace_name(20),
                         "engggui_calibration_bank_20")

    @patch(file_path + ".Ads")
    def test_update_calibration_params_table_retrieves_workspace(self, ads):
        table = [[0, 18414.900000000001, 0.0, -11.82], [1, 18497.75, 0.0, -26.5]]

        self.model.update_calibration_params_table(table)

        self.assertEqual(ads.retrieve.call_count, 1)

    @patch(file_path + ".Ads")
    def test_update_calibration_params_table_stops_when_table_empty(self, ads):
        table = []

        self.model.update_calibration_params_table(table)

        self.assertEqual(ads.retrieve.call_count, 0)

    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    @patch(file_path + ".Ads")
    @patch(file_path + ".EnggUtils")
    def test_run_calibration_no_bank_no_spec_nums(self, utils, ads, pdc, clone_ws, load, nbc, adc, conv, df):
        df.side_effect = ["focused_bank_1", "focused_bank_2"]
        ads.retrieve.return_value = MagicMock()
        self.model.run_calibration("sample", None, None, None, "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_bank_1",
                                    OutputCalibrationTable="engggui_calibration_bank_1")
        pdc.assert_any_call_partial(InputWorkspace="focused_bank_2",
                                    OutputCalibrationTable="engggui_calibration_bank_2")
        self.assertEqual(2, pdc.call_count)

    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    @patch(file_path + ".Ads")
    @patch(file_path + ".EnggUtils")
    def test_run_calibration_bank_no_spec_nums(self, utils, ads, pdc, clone_ws, load, nbc, adc, conv, df):
        df.side_effect = ["focused_bank_1"]
        ads.retrieve.return_value = MagicMock()
        self.model.run_calibration("sample", '1', None, None, "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_bank_1",
                                    OutputCalibrationTable="engggui_calibration_bank_1")
        self.assertEqual(1, pdc.call_count)

    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    @patch(file_path + ".Ads")
    @patch(file_path + ".EnggUtils")
    def test_run_calibration_no_bank_spec_nums(self, utils, ads, pdc, clone_ws, load, nbc, adc, conv, df):
        df.side_effect = ["focused_Cropped"]
        ads.retrieve.return_value = MagicMock()
        self.model.run_calibration("sample", None, None, '28-98', "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_Cropped",
                                    OutputCalibrationTable="engggui_calibration_Cropped")
        self.assertEqual(1, pdc.call_count)

    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    @patch(file_path + ".Ads")
    @patch(file_path + ".EnggUtils")
    def test_run_calibration_custom_calfile(self, utils, ads, pdc, clone_ws, load, nbc, adc, conv, df):
        df.side_effect = ["focused_Custom"]
        ads.retrieve.return_value = MagicMock()
        self.model.run_calibration("sample", None, "/stuff/mycalfile.cal", None, "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_Custom",
                                    OutputCalibrationTable="engggui_calibration_Custom")
        self.assertEqual(1, pdc.call_count)


if __name__ == '__main__':
    unittest.main()
