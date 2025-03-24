import unittest
from unittest import mock
from unittest.mock import call, patch, create_autospec, MagicMock
from numpy import array
from os import path

from Engineering.common.calibration_info import CalibrationInfo
from Engineering.EnggUtils import (
    read_diff_constants_from_prm,
    create_output_files,
    _save_output_files,
    _load_run_and_convert_to_dSpacing,
    process_vanadium,
)

enggutils_path = "Engineering.EnggUtils"


class EnggUtilsTest(unittest.TestCase):
    def setUp(self):
        self.calibration = create_autospec(CalibrationInfo(), instance=True)
        self.calibration.is_valid.return_value = True
        self.calibration.get_instrument.return_value = "ENGINX"
        self.calibration.get_group_suffix.return_value = "all_banks"
        self.calibration.get_foc_ws_suffix.return_value = "bank"

    # tests for code used in calibration tab of UI

    @patch(enggutils_path + ".copy2")
    @patch(enggutils_path + ".path")
    @patch(enggutils_path + ".mantid.SaveNexus")
    @patch(enggutils_path + ".write_prm_file")
    def test_create_output_files_saves_custom_group_file(self, mock_write_prm, mock_save_nxs, mock_path, mock_copy):
        mock_path.exists.return_value = True
        prm_fname = "prm.prm"
        mock_path.join.return_value = prm_fname
        mock_path.splitext.return_value = (prm_fname.replace(".prm", ""), None)
        self.calibration.group.banks = None  # no bank data e.g. custom
        self.calibration.generate_output_file_name.return_value = prm_fname
        self.calibration.get_calibration_table.return_value = "cal_table"  # no bank data e.g. custom
        save_dir = "savedir"

        create_output_files(save_dir, self.calibration, "ws")

        self.calibration.save_grouping_workspace.assert_called_once_with(save_dir)
        mock_write_prm.assert_called_once_with("ws", prm_fname)
        mock_save_nxs.assert_called_once_with(InputWorkspace="cal_table", Filename=prm_fname.replace(".prm", ".nxs"))
        mock_copy.assert_not_called()

    @patch(enggutils_path + ".copy2")
    @patch(enggutils_path + ".makedirs")
    @patch(enggutils_path + ".path.exists")
    @patch(enggutils_path + ".mantid.SaveNexus")
    @patch(enggutils_path + ".write_prm_file")
    def test_create_output_files_makes_savdir_and_saves_both_banks(self, mock_write_prm, mock_save_nxs, mock_exists, mock_mkdir, mock_copy):
        mock_exists.return_value = False  # make new directory
        calibration = CalibrationInfo()  # easier to work with real calibration info object here
        prm_name = "ENGINX_193749_all_banks.prm"
        calibration.set_calibration_from_prm_fname(prm_name)
        calibration.set_calibration_table("cal_table")
        save_dir = "savedir"

        create_output_files(save_dir, calibration, "ws")

        mock_mkdir.assert_called_once_with(save_dir)
        self.calibration.save_grouping_workspace.assert_not_called()  # only called if not bank data
        prm_fpath = path.join(save_dir, prm_name)
        write_prm_calls = [
            call("ws", prm_fpath),
            call("ws", prm_fpath.replace("all_banks", "bank_1"), spec_nums=[0]),
            call("ws", prm_fpath.replace("all_banks", "bank_2"), spec_nums=[1]),
        ]
        mock_write_prm.assert_has_calls(write_prm_calls)
        nxs_fpath = prm_fpath.replace(".prm", ".nxs")
        mock_save_nxs.assert_called_once_with(InputWorkspace="cal_table", Filename=nxs_fpath)
        copy_calls = [call(nxs_fpath, nxs_fpath.replace("all_banks", "bank_1")), call(nxs_fpath, nxs_fpath.replace("all_banks", "bank_2"))]
        mock_copy.assert_has_calls(copy_calls)

    def test_read_diff_constants_from_prm(self):
        file_content = """ID    ENGIN-X CALIBRATION WITH CeO2 and V-Nb
INS    CALIB   241391   ceo2
INS  1 ICONS  18306.98      2.99     14.44
INS  2 ICONS  18497.75    -29.68    -26.50"""
        mocked_handle = mock.mock_open(read_data=file_content)
        dummy_file_path = "/foo/bar_123.prm"
        patchable = "builtins.open"
        with mock.patch(patchable, mocked_handle):
            diff_consts = read_diff_constants_from_prm(dummy_file_path)
        deltas = abs(diff_consts - array([[2.99, 18306.98, 14.44], [-29.68, 18497.75, -26.5]]))
        self.assertTrue((deltas < 1e-10).all())

    # tests for code used in focus tab of UI

    @patch(enggutils_path + ".path_handling.get_run_number_from_path")
    @patch(enggutils_path + ".ADS")
    def test_process_vanadium_foc_curves_exist(self, mock_ads, mock_path):
        mock_path.return_value = "123456"
        mock_ads.doesExist.return_value = True  # foc vanadium exist
        mock_ads.retrieve.return_value = "van_ws_foc"

        ws_van_foc, van_run = process_vanadium("van_path", self.calibration, "full_calib")

        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(enggutils_path + "._smooth_vanadium")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + ".path_handling.get_run_number_from_path")
    @patch(enggutils_path + ".ADS")
    def test_process_vanadium_run_exists_not_focused_over_ROI(self, mock_ads, mock_path, mock_load_run, mock_foc_run, mock_smooth_van):
        mock_path.return_value = "123456"
        mock_ads.doesExist.side_effect = [False, True]  # foc vanadium not exist but original van ws does
        mock_smooth_van.return_value = "van_ws_foc"  # last alg called before return

        ws_van_foc, van_run = process_vanadium("van_path", self.calibration, "full_calib")

        mock_ads.retrieve.assert_called_once_with("123456")
        mock_load_run.assert_not_called()
        mock_foc_run.assert_called_once()
        mock_smooth_van.assert_called_once()
        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(enggutils_path + "._smooth_vanadium")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + ".path_handling.get_run_number_from_path")
    @patch(enggutils_path + ".ADS")
    def test_process_vanadium_run_not_loaded(self, mock_ads, mock_path, mock_load_run, mock_foc_run, mock_smooth_van):
        mock_path.return_value = "123456"
        mock_ads.doesExist.side_effect = [False, False]  # vanadium run not loaded
        mock_smooth_van.return_value = "van_ws_foc"  # last alg called before return

        ws_van_foc, van_run = process_vanadium("van_path", self.calibration, "full_calib")

        mock_ads.retrieve.assert_not_called()
        mock_load_run.assert_called_once()
        mock_foc_run.assert_called_once()
        mock_smooth_van.assert_called_once()
        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + ".mantid.NormaliseByCurrent")
    @patch(enggutils_path + ".logger")
    @patch(enggutils_path + ".path_handling.get_run_number_from_path")
    @patch(enggutils_path + ".mantid.Load")
    def test_load_runs_ignores_empty_runs_with_zeros_charge(self, mock_load, mock_path, mock_log, mock_norm, mock_del):
        ws = MagicMock()
        ws.getRun.return_value = MagicMock()
        ws.getRun().getProtonCharge.return_value = 0  # zero proton charge -> empty run to be ignored
        mock_load.return_value = ws

        ws_foc = _load_run_and_convert_to_dSpacing("fpath", "instrument", "full_calib")

        self.assertIsNone(ws_foc)
        mock_log.warning.assert_called_once()
        mock_norm.assert_not_called()  # throws error if zero charge
        mock_del.assert_called_once()

    @patch(enggutils_path + ".path.exists")
    @patch(enggutils_path + ".mantid.SaveFocusedXYE")
    @patch(enggutils_path + ".mantid.SaveGSS")
    @patch(enggutils_path + ".mantid.SaveNexus")
    @patch(enggutils_path + ".mantid.AddSampleLog")
    def test_save_output_files_both_banks_no_RB_number_path_exists(
        self, mock_add_log, mock_save_nxs, mock_save_gss, mock_save_xye, mock_path
    ):
        mock_path.return_value = True  # directory exists
        ws_foc = MagicMock()
        ws_foc.getNumberHistograms.return_value = 2
        ws_foc.getDimension.return_value = MagicMock()
        ws_foc.getDimension().name = "Time-of-flight"  # x-unit
        ws_foc.run.return_value = MagicMock()
        ws_foc.run().get.return_value = MagicMock().value
        ws_foc.run().get().value = "193749"  # runno
        van_run = "123456"

        focused_files = _save_output_files(["save_dir"], ws_foc, self.calibration, van_run, rb_num=None)

        mock_save_gss.assert_called_once()
        mock_save_xye.assert_called_once()
        add_log_calls = [
            call(Workspace=ws_foc, LogName="Vanadium Run", LogText=van_run),
            call(Workspace=ws_foc, LogName="bankid", LogText="bank 1"),
            call(Workspace=ws_foc, LogName="bankid", LogText="bank 2"),
        ]
        mock_add_log.assert_has_calls(add_log_calls)
        save_nxs_calls = [
            call(InputWorkspace=ws_foc, Filename=focused_files[0][0], WorkspaceIndexList=[0]),
            call(InputWorkspace=ws_foc, Filename=focused_files[0][1], WorkspaceIndexList=[1]),
        ]
        mock_save_nxs.assert_has_calls(save_nxs_calls)

    @patch(enggutils_path + ".makedirs")
    @patch(enggutils_path + ".path.exists")
    @patch(enggutils_path + ".mantid.SaveFocusedXYE")
    @patch(enggutils_path + ".mantid.SaveGSS")
    @patch(enggutils_path + ".mantid.SaveNexus")
    @patch(enggutils_path + ".mantid.AddSampleLog")
    def test_save_output_files_North_Bank_RB_number_path_not_exists(
        self, mock_add_log, mock_save_nxs, mock_save_gss, mock_save_xye, mock_path, mock_mkdir
    ):
        self.calibration.get_group_suffix.return_value = "bank_1"
        self.calibration.get_foc_ws_suffix.return_value = "bank_1"
        mock_path.return_value = False  # directory exists
        ws_foc = MagicMock()
        ws_foc.getNumberHistograms.return_value = 1
        ws_foc.getDimension.return_value = MagicMock()
        ws_foc.getDimension().name = "Time-of-flight"  # x-unit
        ws_foc.run.return_value = MagicMock()
        ws_foc.run().get.return_value = MagicMock().value
        ws_foc.run().get().value = "193749"  # runno
        van_run = "123456"
        rb_num = "1"

        focused_files = _save_output_files(["save_dir"], ws_foc, self.calibration, van_run, rb_num=rb_num)

        mock_mkdir.assert_called_once()
        mock_save_gss.assert_called_once()
        mock_save_xye.assert_called_once()
        add_log_calls = [
            call(Workspace=ws_foc, LogName="Vanadium Run", LogText=van_run),
            call(Workspace=ws_foc, LogName="bankid", LogText="bank 1"),
        ]
        mock_add_log.assert_has_calls(add_log_calls)
        mock_save_nxs.assert_called_once_with(InputWorkspace=ws_foc, Filename=focused_files[0][0], WorkspaceIndexList=[0])


if __name__ == "__main__":
    unittest.main()
