# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import shutil

from unittest.mock import patch, MagicMock, call, create_autospec
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus import model
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

file_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus.model"


class FocusModelTest(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.model = model.FocusModel()
        self.calibration = create_autospec(CalibrationInfo)
        self.calibration.is_valid.return_value = True
        self.calibration.get_instrument.return_value = "ENGINX"
        self.calibration.get_group_suffix.return_value = "all_banks"
        self.calibration.get_foc_ws_suffix.return_value = "bank"

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    @patch(file_path + '.DeleteWorkspace')
    @patch(file_path + '.ConvertUnits')
    @patch(file_path + '.FocusModel._save_output_files')
    @patch(file_path + '.FocusModel._apply_vanadium_norm')
    @patch(file_path + '.FocusModel._focus_run_and_apply_roi_calibration')
    @patch(file_path + '.FocusModel._load_run_and_convert_to_dSpacing')
    @patch(file_path + '.FocusModel._plot_focused_workspaces')
    @patch(file_path + '.FocusModel.process_vanadium')
    @patch(file_path + '.load_full_instrument_calibration')
    def test_focus_run_valid_calibration_plotting(self, mock_load_inst_cal, mock_proc_van, mock_plot, mock_load_run,
                                                  mock_foc_run, mock_apply_van, mock_save_out, mock_conv_units,
                                                  mock_del_ws):
        mock_proc_van.return_value = ("van_ws_foc", "123456")
        mock_load_run.return_value = MagicMock()
        sample_foc_ws = MagicMock()
        sample_foc_ws.name.return_value = "foc_name"
        mock_conv_units.return_value = sample_foc_ws  # last alg called before ws name appended to plotting list

        # plotting focused runs
        self.model.focus_run(["305761"], "fake/van/path", plot_output=True, rb_num=None, calibration=self.calibration)

        mock_foc_run.assert_called_once()
        mock_plot.assert_called_once_with(["foc_name"])
        self.assertEqual(mock_save_out.call_count, 2)  # once for dSpacing and once for TOF
        mock_del_ws.assert_called_once_with("van_ws_foc_rb")

        # no plotting
        mock_plot.reset_mock()
        self.model.focus_run(["305761"], "fake/van/path", plot_output=False, rb_num=None, calibration=self.calibration)

        mock_plot.assert_not_called()

    @patch(file_path + '.DeleteWorkspace')
    @patch(file_path + '.FocusModel._save_output_files')
    @patch(file_path + '.FocusModel._load_run_and_convert_to_dSpacing')
    @patch(file_path + '.FocusModel._plot_focused_workspaces')
    @patch(file_path + '.FocusModel.process_vanadium')
    @patch(file_path + '.load_full_instrument_calibration')
    def test_focus_run_when_zero_proton_charge(self, mock_load_inst_cal, mock_proc_van, mock_plot, mock_load_run,
                                               mock_save_out, mock_del_ws):
        mock_proc_van.return_value = ("van_ws_foc", "123456")
        mock_load_run.return_value = None  # expected return when no proton charge

        self.model.focus_run(["305761"], "fake/van/path", plot_output=True, rb_num=None, calibration=self.calibration)

        mock_plot.assert_not_called()
        mock_save_out.assert_not_called()
        mock_del_ws.assert_not_called()

    @patch(file_path + '.path_handling.get_run_number_from_path')
    @patch(file_path + '.Ads')
    def test_process_vanadium_foc_curves_exist(self, mock_ads, mock_path):
        mock_path.return_value = "123456"
        mock_ads.doesExist.return_value = True  # foc vanadium exist
        mock_ads.retrieve.return_value = "van_ws_foc"

        ws_van_foc, van_run = self.model.process_vanadium("van_path", self.calibration, "full_calib")

        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(file_path + '.FocusModel._smooth_vanadium')
    @patch(file_path + '.FocusModel._focus_run_and_apply_roi_calibration')
    @patch(file_path + '.FocusModel._load_run_and_convert_to_dSpacing')
    @patch(file_path + '.path_handling.get_run_number_from_path')
    @patch(file_path + '.Ads')
    def test_process_vanadium_run_exists_not_focused_over_ROI(self, mock_ads, mock_path, mock_load_run,
                                                              mock_foc_run, mock_smooth_van):
        mock_path.return_value = "123456"
        mock_ads.doesExist.side_effect = [False, True]  # foc vanadium not exist but original van ws does
        mock_smooth_van.return_value = "van_ws_foc"  # last alg called before return

        ws_van_foc, van_run = self.model.process_vanadium("van_path", self.calibration, "full_calib")

        mock_ads.retrieve.assert_called_once_with("123456")
        mock_load_run.assert_not_called()
        mock_foc_run.assert_called_once()
        mock_smooth_van.assert_called_once()
        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(file_path + '.FocusModel._smooth_vanadium')
    @patch(file_path + '.FocusModel._focus_run_and_apply_roi_calibration')
    @patch(file_path + '.FocusModel._load_run_and_convert_to_dSpacing')
    @patch(file_path + '.path_handling.get_run_number_from_path')
    @patch(file_path + '.Ads')
    def test_process_vanadium_run_not_loaded(self, mock_ads, mock_path, mock_load_run, mock_foc_run, mock_smooth_van):
        mock_path.return_value = "123456"
        mock_ads.doesExist.side_effect = [False, False]  # vanadium run not loaded
        mock_smooth_van.return_value = "van_ws_foc"  # last alg called before return

        ws_van_foc, van_run = self.model.process_vanadium("van_path", self.calibration, "full_calib")

        mock_ads.retrieve.assert_not_called()
        mock_load_run.assert_called_once()
        mock_foc_run.assert_called_once()
        mock_smooth_van.assert_called_once()
        self.assertEqual(ws_van_foc, "van_ws_foc")
        self.assertEqual(van_run, "123456")

    @patch(file_path + '.DeleteWorkspace')
    @patch(file_path + '.NormaliseByCurrent')
    @patch(file_path + '.logger')
    @patch(file_path + '.path_handling.get_run_number_from_path')
    @patch(file_path + '.Load')
    def test_load_runs_ignores_empty_runs_with_zeros_charge(self, mock_load, mock_path, mock_log, mock_norm, mock_del):
        ws = MagicMock()
        ws.getRun.return_value = MagicMock()
        ws.getRun().getProtonCharge.return_value = 0  # zero proton charge -> empty run to be ignored
        mock_load.return_value = ws

        ws_foc = self.model._load_run_and_convert_to_dSpacing("fpath", "instrument", "full_calib")

        self.assertIsNone(ws_foc)
        mock_log.warning.assert_called_once()
        mock_norm.assert_not_called()  # throws error if zero charge
        mock_del.assert_called_once()

    @patch(file_path + '.output_settings')
    @patch(file_path + '.path.exists')
    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_save_output_files_both_banks_no_RB_number_path_exists(self, mock_add_log, mock_save_nxs, mock_save_gss,
                                                                   mock_save_xye, mock_path, mock_out_setting):
        mock_out_setting.get_output_path.return_value = "dir"
        mock_path.return_value = True  # directory exists
        ws_foc = MagicMock()
        ws_foc.getNumberHistograms.return_value = 2
        ws_foc.getDimension.return_value = MagicMock()
        ws_foc.getDimension().name = 'Time-of-flight'  # x-unit
        ws_foc.run.return_value = MagicMock()
        ws_foc.run().get.return_value = MagicMock().value
        ws_foc.run().get().value = '193749'  # runno
        van_run = '123456'

        self.model._save_output_files(ws_foc, self.calibration, van_run, rb_num=None)

        mock_save_gss.assert_called_once()
        mock_save_xye.assert_called_once()
        add_log_calls = [call(Workspace=ws_foc, LogName="Vanadium Run", LogText=van_run),
                         call(Workspace=ws_foc, LogName="bankid", LogText='bank 1'),
                         call(Workspace=ws_foc, LogName="bankid", LogText='bank 2')]
        mock_add_log.assert_has_calls(add_log_calls)
        save_nxs_calls = [call(InputWorkspace=ws_foc, Filename=self.model._last_focused_files[0],
                               WorkspaceIndexList=[0]),
                          call(InputWorkspace=ws_foc, Filename=self.model._last_focused_files[1],
                               WorkspaceIndexList=[1])]
        mock_save_nxs.assert_has_calls(save_nxs_calls)

    @patch(file_path + '.makedirs')
    @patch(file_path + '.output_settings')
    @patch(file_path + '.path.exists')
    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_save_output_files_North_Bank_RB_number_path_not_exists(self, mock_add_log, mock_save_nxs, mock_save_gss,
                                                                    mock_save_xye, mock_path, mock_out_setting,
                                                                    mock_mkdir):
        self.calibration.get_group_suffix.return_value = "bank_1"
        self.calibration.get_foc_ws_suffix.return_value = "bank_1"
        mock_out_setting.get_output_path.return_value = "dir"
        mock_path.return_value = False  # directory exists
        ws_foc = MagicMock()
        ws_foc.getNumberHistograms.return_value = 1
        ws_foc.getDimension.return_value = MagicMock()
        ws_foc.getDimension().name = 'Time-of-flight'  # x-unit
        ws_foc.run.return_value = MagicMock()
        ws_foc.run().get.return_value = MagicMock().value
        ws_foc.run().get().value = '193749'  # runno
        van_run = '123456'
        rb_num = '1'

        self.model._save_output_files(ws_foc, self.calibration, van_run, rb_num=rb_num)

        mock_mkdir.assert_called_once()
        mock_save_gss.assert_called_once()
        mock_save_xye.assert_called_once()
        add_log_calls = [call(Workspace=ws_foc, LogName="Vanadium Run", LogText=van_run),
                         call(Workspace=ws_foc, LogName="bankid", LogText='bank 1')]
        mock_add_log.assert_has_calls(add_log_calls)
        mock_save_nxs.assert_called_once_with(InputWorkspace=ws_foc, Filename=self.model._last_focused_files[0],
                                              WorkspaceIndexList=[0])


if __name__ == '__main__':
    unittest.main()
