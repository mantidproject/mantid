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

from unittest.mock import patch, MagicMock, call
from mantid.simpleapi import CreateSampleWorkspace
from Engineering.gui.engineering_diffraction.tabs.focus import model
from Engineering.gui.engineering_diffraction.tabs.common import output_settings
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

file_path = "Engineering.gui.engineering_diffraction.tabs.focus.model"

DF_KWARG_NORTH = {'GroupingWorkspace': 'NorthBank_grouping'}
DF_KWARG_SOUTH = {'GroupingWorkspace': 'SouthBank_grouping'}
DF_KWARG_CUSTOM = {'GroupingWorkspace': 'custom_grouping_wsp'}


def clone_ws_side_effect(ws):
    return ws


class FocusModelTest(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.model = model.FocusModel()
        self.current_calibration = CalibrationInfo(sample_path="this_is_mocked_out_too",
                                                   instrument="ENGINX")

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_run_for_each_bank(self, fetch_van, load_focus, run_focus, prefocus, output, ads, logs, load, delete):
        ads.retrieve.side_effect = ["full_calib", "calib_n", "curves_n", "calib_s", "curves_s"]
        regions_dict = {"bank_1": "NorthBank_grouping", "bank_2": "SouthBank_grouping"}
        load_focus.return_value = "mocked_sample"
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        van_path = "fake/van/path"

        self.model.focus_run(["305761"], van_path, False, "ENGINX", "0", regions_dict)

        self.assertEqual(2, run_focus.call_count)
        north_call = call("mocked_sample",
                          "305761_" + model.FOCUSED_OUTPUT_WORKSPACE_NAME + "bank_1",
                          "curves_n", "NorthBank_grouping", "calib_n")
        south_call = call("mocked_sample",
                          "305761_" + model.FOCUSED_OUTPUT_WORKSPACE_NAME + "bank_2",
                          "curves_s", "SouthBank_grouping", "calib_s")
        run_focus.assert_has_calls([north_call, south_call])

    @patch(file_path + ".DeleteWorkspace")
    @patch(file_path + ".Load")
    @patch(file_path + ".FocusModel._output_sample_logs")
    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_output")
    @patch(file_path + ".FocusModel._whole_inst_prefocus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".path_handling.load_workspace")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_run_for_custom_spectra(self, fetch_van, load_focus, run_focus, prefocus, output, ads, logs, load,
                                          delete):
        ads.retrieve.side_effect = ["full_calib", "calib_cropped", "curves_cropped"]
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        van_path = "fake/van/path"
        load_focus.return_value = "mocked_sample"
        regions_dict = {"Cropped": "Custom_spectra_grouping"}

        self.model.focus_run(["305761"], van_path, False, "ENGINX", "0", regions_dict)

        self.assertEqual(1, run_focus.call_count)
        run_focus.assert_called_with("mocked_sample",
                                     "305761_" + model.FOCUSED_OUTPUT_WORKSPACE_NAME + "Cropped",
                                     "curves_cropped", "Custom_spectra_grouping", "calib_cropped")

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
        van_path = "fake/van/path"
        regions_dict = {"bank_1": "NorthBank_grouping", "bank_2": "SouthBank_grouping"}
        load_focus.return_value = "mocked_sample"

        self.model.focus_run(["305761"], van_path, True, "ENGINX", "0", regions_dict)

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
        van_path = "fake/van/path"
        regions_dict = {"region1": "grp_ws_name"}
        load_focus.return_value = "mocked_sample"

        self.model.focus_run("305761", van_path, False, "ENGINX", "0", regions_dict)
        self.assertEqual(0, plot_focus.call_count)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_save_output_files_with_no_RB_number(self, addlog, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        output_file = path.join(output_settings.get_output_path(), "Focus",
                                "ENGINX_123_456_North_TOF.nxs")

        self.model._save_output("ENGINX", "123", "456", "North",
                                mocked_workspace, None)

        self.assertEqual(1, nexus.call_count)
        self.assertEqual(1, gss.call_count)
        self.assertEqual(1, xye.call_count)
        nexus.assert_called_with(Filename=output_file, InputWorkspace=mocked_workspace)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_save_output_files_with_RB_number(self, addlog, nexus, gss, xye):
        self.model._save_output("ENGINX", "123", "456", "North",
                                "mocked-workspace", "An Experiment Number")
        self.assertEqual(nexus.call_count, 2)
        self.assertEqual(gss.call_count, 2)
        self.assertEqual(xye.call_count, 2)

    @patch(file_path + ".logger")
    @patch(file_path + ".output_settings.get_output_path")
    @patch(file_path + ".csv")
    def test_output_sample_logs_with_rb_number(self, mock_csv, mock_path, mock_logger):
        mock_writer = MagicMock()
        mock_csv.writer.return_value = mock_writer
        mock_path.return_value = self.test_dir
        ws = CreateSampleWorkspace()

        self.model._output_sample_logs("ENGINX", "00000", "00000", ws, "0")

        self.assertEqual(5, len(ws.getRun().keys()))
        self.assertEqual(1, mock_logger.information.call_count)
        self.assertEqual(8, mock_writer.writerow.call_count)

    @patch(file_path + ".logger")
    @patch(file_path + ".output_settings.get_output_path")
    @patch(file_path + ".csv")
    def test_output_sample_logs_without_rb_number(self, mock_csv, mock_path, mock_logger):
        mock_writer = MagicMock()
        mock_csv.writer.return_value = mock_writer
        mock_path.return_value = self.test_dir
        ws = CreateSampleWorkspace()

        self.model._output_sample_logs("ENGINX", "00000", "00000", ws, None)

        self.assertEqual(5, len(ws.getRun().keys()))
        self.assertEqual(1, mock_logger.information.call_count)
        self.assertEqual(4, mock_writer.writerow.call_count)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_last_path_updates_with_no_RB_number(self, addlog, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        output_file = path.join(output_settings.get_output_path(), "Focus",
                                "ENGINX_123_456_North_TOF.nxs")

        self.model._save_output("ENGINX", "123", "456", "North",
                                mocked_workspace, None)

        self.assertEqual(self.model._last_focused_files[0], output_file)

    @patch(file_path + ".SaveFocusedXYE")
    @patch(file_path + ".SaveGSS")
    @patch(file_path + ".SaveNexus")
    @patch(file_path + ".AddSampleLog")
    def test_last_path_updates_with_RB_number(self, addlog, nexus, gss, xye):
        mocked_workspace = "mocked-workspace"
        rb_num = '2'
        output_file = path.join(output_settings.get_output_path(), "User", rb_num, "Focus",
                                "ENGINX_123_456_North_TOF.nxs")

        self.model._save_output("ENGINX", "123", "456", "North",
                                mocked_workspace, rb_num)

        self.assertEqual(self.model._last_focused_files[0], output_file)


if __name__ == '__main__':
    unittest.main()
