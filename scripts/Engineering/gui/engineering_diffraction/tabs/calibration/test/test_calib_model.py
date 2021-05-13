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
INTEGRATED_WORKSPACE_NAME = "engggui_vanadium_integration"
CURVES_WORKSPACE_NAME = "engggui_vanadium_curves"
INPUT_WORKSPACE_NAME = "engggui_vanadium_ws"
TEST_PRM_FILE = "ENGINX_241391_236516_testall_banks.prm"
class_path = "Engineering.gui.engineering_diffraction.tabs.calibration.model.CalibrationModel"
file_path = "Engineering.gui.engineering_diffraction.tabs.calibration.model"


def _convert_units_returns(bank):
    if bank == '1':
        return "engggui_calibration_bank_1"
    elif bank == '2':
        return "engggui_calibration_bank_2"
    elif bank == 'cropped':
        return "engggui_calibration_cropped"


def _run_calibration_returns(single_output=False):
    calib = [{"difa": 0, "difc": 0, "tzero": 0}, {"difa": 0, "difc": 0, "tzero": 0}]
    calib_single = [{"difa": 0, "difc": 0, "tzero": 0}]
    curves = [MagicMock(), MagicMock()]
    sample = MagicMock()
    if single_output:
        return calib_single, curves, sample
    return calib, curves, sample


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
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(class_path + ".run_calibration")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_fetch_vanadium_is_called(self, van_corr, calibrate_alg, load_sample, load, handle_vc, output_files,
                                      update_table, delet):
        van_corr.return_value = ("mocked_integration", "mocked_curves")
        calibrate_alg.return_value = _run_calibration_returns()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(van_corr.call_count, 1)

    @patch(class_path + ".get_info_from_file")
    @patch(file_path + ".path")
    @patch(file_path + ".load_relevant_pdcal_outputs")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(class_path + ".update_calibration_params_table")
    def test_load_existing_calibration_files(self, update_table, fetch_ws, load_cals, path, get_info):
        path.exists.return_value = True
        get_info.return_value = "TESTINST", "van_no", "ceria_no", [["params"], ["table"]]
        inst, van_no, ceria_no = self.model.load_existing_calibration_files(TEST_PRM_FILE)

        table_test_params = [["params"], ["table"]]
        update_table.assert_called_with(table_test_params)
        self.assertEqual(1, update_table.call_count)

        fetch_ws.assert_called_with("TESTINSTvan_no", "TESTINST")
        self.assertEqual(1, fetch_ws.call_count)

        self.assertEqual(1, load_cals.call_count)

        self.assertEqual("TESTINST", inst)
        self.assertEqual("van_no", van_no)
        self.assertEqual("ceria_no", ceria_no)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(class_path + "._generate_tof_fit_workspace")
    @patch(class_path + "._plot_tof_fit")
    @patch(class_path + ".run_calibration")
    def test_plotting_check(self, calib, plot_tof, gen_tof, load, handle_vc, van, sample, output_files, update_table,
                            delete):
        calib.return_value = _run_calibration_returns()
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        plot_tof.assert_not_called()
        gen_tof.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX")
        self.assertEqual(gen_tof.call_count, 2)
        self.assertEqual(plot_tof.call_count, 1)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(class_path + "._generate_tof_fit_workspace")
    @patch(class_path + "._plot_tof_fit")
    @patch(class_path + "._plot_tof_fit_single_bank_or_custom")
    @patch(class_path + ".run_calibration")
    def test_plotting_check_cropped(self, calib, plot_tof_cus, plot_tof_fit, gen_tof, load, handle_vc, van, sample,
                                    output_files, update_table, delete):
        calib.return_value = _run_calibration_returns(single_output=True)
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        plot_tof_cus.assert_not_called()
        plot_tof_fit.assert_not_called()
        gen_tof.assert_not_called()
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, True, "ENGINX", bank=1)
        self.assertEqual(gen_tof.call_count, 1)
        plot_tof_fit.assert_not_called()
        self.assertEqual(plot_tof_cus.call_count, 1)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(class_path + "._plot_tof_fit")
    @patch(class_path + ".run_calibration")
    def test_present_RB_number_results_in_user_output_files(self, calib, plot_tof, load, handle_vc, van, sample,
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
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(class_path + "._plot_tof_fit")
    @patch(class_path + ".run_calibration")
    def test_absent_run_number_results_in_no_user_output_files(self, calib, plot_tof, load, handle_vc,
                                                               van, sample, output_files, update_table, delete):
        calib.return_value = _run_calibration_returns()
        van.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(output_files.call_count, 1)

    @patch(file_path + ".DeleteWorkspace")
    @patch(class_path + ".update_calibration_params_table")
    @patch(class_path + ".create_output_files")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".Load")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    @patch(class_path + ".run_calibration")
    def test_calibration_params_table_is_updated(self, calibrate_alg, vanadium_alg, load, handle_vc, load_sample,
                                                 output_files, update_table, delete):
        calibrate_alg.return_value = _run_calibration_returns()
        vanadium_alg.return_value = ("A", "B")
        self.model.create_new_calibration(VANADIUM_NUMBER, CERIUM_NUMBER, False, "ENGINX")
        self.assertEqual(calibrate_alg.call_count, 1)

    @patch(class_path + "._generate_output_file_name")
    @patch(file_path + ".makedirs")
    @patch(file_path + ".write_ENGINX_GSAS_iparam_file")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".Load")
    def test_create_output_files(self, load, saven, write_file, make_dirs, output_name):
        sample_path = "test2/test3/ENGINX20.nxs"
        vanadium_path = "test4/ENGINX0010.nxs"
        filename = "output"
        output_name.return_value = filename

        self.model.create_output_files("test/", [2, 2], [0, 0], [1, 1],
                                       [[0, 1, 2, 3, 4, 5], [0, 1, 2, 3, 4, 5]],
                                       sample_path,
                                       vanadium_path,
                                       "ENGINX",
                                       bank=None,
                                       spectrum_numbers=None)

        self.assertEqual(make_dirs.call_count, 1)
        self.assertEqual(write_file.call_count, 3)
        self.assertEqual(saven.call_count, 2)
        write_file.assert_called_with("test/" + filename, [2], [0], [1],
                                      [[0, 1, 2, 3, 4, 5], [0, 1, 2, 3, 4, 5]],
                                      bank_names=["South"],
                                      ceria_run="20",
                                      template_file="template_ENGINX_241391_236516_South_bank.prm",
                                      vanadium_run="10")

    def test_generate_table_workspace_name(self):
        self.assertEqual(self.model._generate_table_workspace_name(20),
                         "engggui_calibration_bank_20")

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

    def test_generate_output_file_name_for_cropped_bank(self):
        filename = self.model._generate_output_file_name("test/20.raw", "test/10.raw", "ENGINX",
                                                         "cropped")
        self.assertEqual(filename, "ENGINX_20_10_cropped.prm")

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

    @patch(file_path + ".ReplaceSpecialValues")
    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    def test_run_calibration_no_bank_no_spec_nums(self, pdc, clone_ws, handle_vc, load, nbc, adc, conv, dw, df, eefb,
                                                  rsv):
        conv.side_effect = [MagicMock(), MagicMock(), "focused_North", "focused_South"]
        self.model.run_calibration("sample", "vanadium_ws", "vanadium_int", None, None, "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_North",
                                    OutputCalibrationTable="engggui_calibration_bank_1")
        pdc.assert_any_call_partial(InputWorkspace="focused_South",
                                    OutputCalibrationTable="engggui_calibration_bank_2")
        self.assertEqual(2, pdc.call_count)

    @patch(file_path + ".ReplaceSpecialValues")
    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    def test_run_calibration_bank_no_spec_nums(self, pdc, clone_ws, handle_vc, load, nbc, adc, conv, dw, df, eefb, rsv):
        conv.side_effect = [MagicMock(), MagicMock(), "focused_North"]
        self.model.run_calibration("sample", "vanadium_ws", "vanadium_int", '1', None, "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_North",
                                    OutputCalibrationTable="engggui_calibration_bank_1")
        self.assertEqual(1, pdc.call_count)

    @patch(file_path + ".create_custom_grouping_workspace")
    @patch(file_path + ".ReplaceSpecialValues")
    @patch(file_path + ".EnggEstimateFocussedBackground")
    @patch(file_path + ".DiffractionFocussing")
    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".ConvertUnits")
    @patch(file_path + ".ApplyDiffCal")
    @patch(file_path + ".NormaliseByCurrent")
    @patch(file_path + ".Load")
    @patch(file_path + ".vanadium_corrections.handle_van_curves")
    @patch(file_path + ".CloneWorkspace")
    @patch(file_path + ".PDCalibration")
    def test_run_calibration_no_bank_spec_nums(self, pdc, clone_ws, handle_vc, load, nbc, adc, conv, dw, df, eefb,
                                               rsv, cgw):
        conv.side_effect = [MagicMock(), MagicMock(), "focused_Cropped"]
        cgw.return_value = MagicMock()
        self.model.run_calibration("sample", "vanadium_ws", "vanadium_int", None, '28-98', "full_calib_ws")

        pdc.assert_any_call_partial(InputWorkspace="focused_Cropped",
                                    OutputCalibrationTable="engggui_calibration_cropped")
        self.assertEqual(1, pdc.call_count)


if __name__ == '__main__':
    unittest.main()
