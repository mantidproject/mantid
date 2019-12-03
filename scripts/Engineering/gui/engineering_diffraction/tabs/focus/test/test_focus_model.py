# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest
from os import path

from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.tabs.focus import model
from Engineering.gui.engineering_diffraction.tabs.common import path_handling

file_path = "Engineering.gui.engineering_diffraction.tabs.focus.model"


class FocusModelTest(unittest.TestCase):
    def setUp(self):
        self.model = model.FocusModel()
        self.current_calibration = {
            "vanadium_path": "/mocked/out/anyway",
            "ceria_path": "this_is_mocked_out_too"
        }

    @patch(file_path + ".FocusModel._load_focus_sample_run")
    @patch(file_path + ".vanadium_corrections.Ads.doesExist")
    def test_focus_cancelled_if_van_wsp_missing(self, ads_exist, load):
        ads_exist.return_value = False
        self.model.focus_run("307593", ["1", "2"], False, "ENGINX", "0")
        self.assertEqual(load.call_count, 0)

    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_focused_output_files_as_nexus")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".FocusModel._load_focus_sample_run")
    def test_focus_run_for_each_bank(self, load_focus, run_focus, output, ads):
        ads.retrieve.return_value = "test_wsp"
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run("305761", banks, False, "ENGINX", "0")
        self.assertEqual(len(banks), run_focus.call_count)
        run_focus.assert_called_with("mocked_sample",
                                     model.FOCUSED_OUTPUT_WORKSPACE_NAME + banks[-1],
                                     "test_wsp", "test_wsp", banks[-1])

    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_focused_output_files_as_nexus")
    @patch(file_path + ".FocusModel._plot_focused_workspace")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".FocusModel._load_focus_sample_run")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_plotted_when_checked(self, fetch_van, load_focus, run_focus, plot_focus, output, ads):
        ads.doesExist.return_value = True
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run("305761", banks, True, "ENGINX", "0")
        self.assertEqual(len(banks), plot_focus.call_count)

    @patch(file_path + ".Ads")
    @patch(file_path + ".FocusModel._save_focused_output_files_as_nexus")
    @patch(file_path + ".FocusModel._plot_focused_workspace")
    @patch(file_path + ".FocusModel._run_focus")
    @patch(file_path + ".FocusModel._load_focus_sample_run")
    @patch(file_path + ".vanadium_corrections.fetch_correction_workspaces")
    def test_focus_not_plotted_when_not_checked(self, fetch_van, load_focus, run_focus, plot_focus,
                                                output, ads):
        fetch_van.return_value = ("mocked_integ", "mocked_curves")
        banks = ["1", "2"]
        load_focus.return_value = "mocked_sample"

        self.model.focus_run("305761", banks, False, "ENGINX", "0")
        self.assertEqual(0, plot_focus.call_count)

    @patch(file_path + ".SaveNexus")
    def test_save_output_files_nexus_with_no_RB_number(self, save):
        mocked_workspace = "mocked-workspace"
        output_file = path.join(path_handling.OUT_FILES_ROOT_DIR, "Focus",
                                "ENGINX_123_bank_North.nxs")
        self.model._save_focused_output_files_as_nexus("ENGINX", "Path/To/ENGINX000123.whatever",
                                                       "North", mocked_workspace, None)

        self.assertEqual(1, save.call_count)
        save.assert_called_with(Filename=output_file, InputWorkspace=mocked_workspace)

    @patch(file_path + ".SaveNexus")
    def test_save_output_files_nexus_with_RB_number(self, save):
        self.model._save_focused_output_files_as_nexus("ENGINX", "Path/To/ENGINX000123.whatever",
                                                       "North", "mocked-workspace",
                                                       "An Experiment Number")
        self.assertEqual(2, save.call_count)


if __name__ == '__main__':
    unittest.main()
