# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import tempfile
import shutil
from os import path

from unittest.mock import patch, MagicMock
from mantid.simpleapi import CreateSampleWorkspace
from Engineering.gui.engineering_diffraction.tabs.focus import model
from Engineering.gui.engineering_diffraction.tabs.common import path_handling
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

file_path = "Engineering.gui.engineering_diffraction.tabs.focus.model"

DF_KWARG_NORTH = {'GroupingFileName': 'EnginX_NorthBank.cal'}
DF_KWARG_SOUTH = {'GroupingFileName': 'EnginX_SouthBank.cal'}
DF_KWARG_CUSTOM = {'GroupingWorkspace': 'custom_grouping_wsp'}


def clone_ws_side_effect(ws):
    return ws


class FocusModelTest(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.model = model.FocusModel()
        self.current_calibration = CalibrationInfo(vanadium_path="/mocked/out/anyway",
                                                   sample_path="this_is_mocked_out_too",
                                                   instrument="ENGINX")

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    @patch(file_path + ".Load")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.Ads.doesExist")
    def test_focus_cancelled_if_van_wsp_missing(self, ads_exist, load_sample, load):
        ads_exist.return_value = False
        self.model.focus_run("307593", ["1", "2"], False, "ENGINX", "0", None, None)
        self.assertEqual(load_sample.call_count, 0)

    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    def test_focus_run_for_each_bank(self, load_focus, run_focus, prefocus, output, ads, logs, load, delete):
        ads.retrieve.return_value = "test_wsp"
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run(["305761"], banks, False, "ENGINX", "0", None, None)

        self.assertEqual(len(banks), run_focus.call_count)
        run_focus.assert_called_with("mocked_sample",
                                     "305761_" + model.FOCUSED_OUTPUT_WORKSPACE_NAME + banks[-1],
                                     "test_wsp", DF_KWARG_SOUTH, "engggui_calibration_bank_2")

    @patch(file_path + ".create_custom_grouping_workspace")
    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    def test_focus_run_for_custom_spectra(self, load_focus, run_focus, prefocus, output, ads, logs, load, delete, cgw):
        ads.retrieve.return_value = "test_wsp"
        spectra = "20-50"
        load_focus.return_value = "mocked_sample"
        cgw.return_value = "custom_grouping_wsp"

        self.model.focus_run(["305761"], None, False, "ENGINX", "0", spectra, None)

        self.assertEqual(1, run_focus.call_count)
        run_focus.assert_called_with("mocked_sample",
                                     "305761_" + model.FOCUSED_OUTPUT_WORKSPACE_NAME + "Cropped",
                                     "test_wsp", DF_KWARG_CUSTOM, "engggui_calibration_Cropped")

    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._plot_focused_workspaces")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_plotted_when_checked(self, fetch_van, load_focus, run_focus, prefocus, plot_focus, output, ads, logs,
                                        load, delete):
        ads.doesExist.return_value = True
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run(["305761"], banks, True, "ENGINX", "0", None, None)

        self.assertEqual(1, plot_focus.call_count)

    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._plot_focused_workspaces")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_not_plotted_when_not_checked(self, fetch_van, load_focus, run_focus, prefocus, plot_focus, output,
                                                ads, logs, load, delete):
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run("305761", banks, False, "ENGINX", "0", None, None)
        self.assertEqual(0, plot_focus.call_count)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    def test_save_output_files_with_no_RB_number(self, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        output_file = path.join(path_handling.get_output_path(), "Focus",
                                "ENGINX_123_bank_North_TOF.nxs")

        self.model._save_output("ENGINX", "Path/To/ENGINX000123.whatever", "North",
                                mocked_workspace, None)

        self.assertEqual(1, nexus.call_count)
        self.assertEqual(1, gss.call_count)
        self.assertEqual(1, xye.call_count)
        nexus.assert_called_with(Filename=output_file, InputWorkspace=mocked_workspace)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    def test_save_output_files_with_RB_number(self, nexus, gss, xye):
        self.model._save_output("ENGINX", "Path/To/ENGINX000123.whatever", "North",
                                "mocked-workspace", "An Experiment Number")
        self.assertEqual(nexus.call_count, 2)
        self.assertEqual(gss.call_count, 2)
        self.assertEqual(xye.call_count, 2)

    @patch(file_path + ".logger")
    @patch(file_path + ".path_handling.get_output_path")
    @patch(file_path + ".csv")
    def test_output_sample_logs_with_rb_number(self, mock_csv, mock_path, mock_logger):
        mock_writer = MagicMock()
        mock_csv.writer.return_value = mock_writer
        mock_path.return_value = self.test_dir
        ws = CreateSampleWorkspace()

        self.model._output_sample_logs("ENGINX", "00000", ws, "0")

        self.assertEqual(5, len(ws.getRun().keys()))
        self.assertEqual(1, mock_logger.information.call_count)
        self.assertEqual(8, mock_writer.writerow.call_count)

    @patch(file_path + ".logger")
    @patch(file_path + ".path_handling.get_output_path")
    @patch(file_path + ".csv")
    def test_output_sample_logs_without_rb_number(self, mock_csv, mock_path, mock_logger):
        mock_writer = MagicMock()
        mock_csv.writer.return_value = mock_writer
        mock_path.return_value = self.test_dir
        ws = CreateSampleWorkspace()

        self.model._output_sample_logs("ENGINX", "00000", ws, None)

        self.assertEqual(5, len(ws.getRun().keys()))
        self.assertEqual(1, mock_logger.information.call_count)
        self.assertEqual(4, mock_writer.writerow.call_count)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    def test_last_path_updates_with_no_RB_number(self, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        output_file = path.join(path_handling.get_output_path(), "Focus",
                                "ENGINX_123_bank_North_TOF.nxs")

        self.model._last_path_ws = 'ENGINX_123_bank_North_TOF.nxs'
        self.model._save_output("ENGINX", "Path/To/ENGINX000123.whatever", "North",
                                mocked_workspace, None)

        self.assertEqual(self.model._last_path, output_file)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    def test_last_path_updates_with_RB_number(self, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        rb_num = '2'
        output_file = path.join(path_handling.get_output_path(), "User", rb_num, "Focus",
                                "ENGINX_123_bank_North_TOF.nxs")

        self.model._last_path_ws = 'ENGINX_123_bank_North_TOF.nxs'
        self.model._save_output("ENGINX", "Path/To/ENGINX000123.whatever", "North",
                                mocked_workspace, rb_num)

        self.assertEqual(self.model._last_path, output_file)


if __name__ == '__main__':
    unittest.main()
