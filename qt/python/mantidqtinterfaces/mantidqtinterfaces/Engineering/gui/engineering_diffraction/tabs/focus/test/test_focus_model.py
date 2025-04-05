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

from unittest import mock
from unittest.mock import patch, MagicMock, call, create_autospec
from Engineering.EnggUtils import GROUP
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus import model
from Engineering.common.calibration_info import CalibrationInfo
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings import settings_model, settings_view, settings_presenter
from qtpy.QtCore import QCoreApplication
from workbench.config import APPNAME

file_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus.model"
enggutils_path = "Engineering.EnggUtils"


class FocusModelTest(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.model = model.FocusModel()
        self.calibration = create_autospec(CalibrationInfo, instance=True)
        self.calibration.is_valid.return_value = True
        self.calibration.get_instrument.return_value = "ENGINX"
        self.calibration.get_group_suffix.return_value = "all_banks"
        self.calibration.get_foc_ws_suffix.return_value = "bank"

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + ".mantid.ConvertUnits")
    @patch(enggutils_path + "._save_output_files")
    @patch(enggutils_path + "._apply_vanadium_norm")
    @patch(enggutils_path + "._check_ws_foc_and_ws_van_foc")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + "._plot_focused_workspaces")
    @patch(enggutils_path + ".process_vanadium")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_focus_run_valid_calibration_plotting(
        self,
        mock_load_inst_cal,
        mock_proc_van,
        mock_plot,
        mock_load_run,
        mock_foc_run,
        mock_check_foc_and_van_foc,
        mock_apply_van,
        mock_save_out,
        mock_conv_units,
        mock_del_ws,
    ):
        mock_proc_van.return_value = ("van_ws_foc", "123456")
        mock_load_run.return_value = MagicMock()
        sample_foc_ws = MagicMock()
        sample_foc_ws.name.return_value = "foc_name"
        mock_conv_units.return_value = sample_foc_ws  # last alg called before ws name appended to plotting list
        mock_save_out.return_value = ["Nexus files"], ["GSS files"]

        # plotting focused runs
        self.model.focus_run(["305761"], "fake/van/path", plot_output=True, rb_num=None, calibration=self.calibration)

        mock_foc_run.assert_called_once()
        mock_plot.assert_called_once_with(["foc_name"])
        self.assertEqual(mock_save_out.call_count, 2)  # once for dSpacing and once for TOF
        self.assertEqual(len(self.model._last_focused_files), 1)
        self.assertEqual(self.model._last_focused_files[0], "Nexus files")
        self.assertEqual(len(self.model._last_focused_files_gsas2), 1)
        self.assertEqual(self.model._last_focused_files_gsas2[0], "GSS files")
        mock_del_ws.assert_called_once_with("van_ws_foc_rb")

        # no plotting
        mock_plot.reset_mock()
        self.model.focus_run(["305761"], "fake/van/path", plot_output=False, rb_num=None, calibration=self.calibration)

        self.assertEqual(len(self.model._last_focused_files), 1)
        self.assertEqual(self.model._last_focused_files[0], "Nexus files")

        mock_plot.assert_not_called()

    @patch(file_path + ".output_settings.get_output_path")
    @patch(enggutils_path + ".focus_run")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_first_time_focus_uses_correct_default_save_directory(self, mock_load_cal, mock_enggutils_focus_run, mock_get_output_path):
        default_save_location = path.join(path.expanduser("~"), "Engineering_Mantid")
        QCoreApplication.setApplicationName("Engineering_Diffraction_test_calib_model")
        presenter = settings_presenter.SettingsPresenter(
            mock.create_autospec(settings_model.SettingsModel, instance=True),
            mock.create_autospec(settings_view.SettingsView, instance=True),
        )
        presenter.settings = {  # "save_location" is not defined
            "full_calibration": "cal",
            "logs": "some,logs",
            "primary_log": "some",
            "sort_ascending": True,
            "default_peak": "BackToBackExponential",
        }
        presenter._validate_settings()  # save_location now set to the default value at runtime
        self.assertEqual(presenter.settings["save_location"], default_save_location)

        # this is the runtime return from output_settings.get_output_path()
        # if called at define time in a default parameter value then this value is not used
        mock_get_output_path.return_value = default_save_location

        mock_load_cal.return_value = "full_calibration"
        self.calibration.group = GROUP.BOTH
        mock_enggutils_focus_run.return_value = ["Nexus files"], ["GSS files"]

        self.model.focus_run(
            ["305761"], "fake/van/path", plot_output=False, rb_num=None, calibration=self.calibration
        )  # save_dir not given

        mock_enggutils_focus_run.assert_called_once_with(
            ["305761"], "fake/van/path", False, None, self.calibration, default_save_location, "full_calibration"
        )
        # sample_paths, vanadium_path, plot_output, rb_num, calibration, save_dir, full_calib
        QCoreApplication.setApplicationName(APPNAME)  # reset to 'mantidworkbench' in case required by other tests

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + ".mantid.ConvertUnits")
    @patch(enggutils_path + "._save_output_files")
    @patch(enggutils_path + "._apply_vanadium_norm")
    @patch(enggutils_path + "._check_ws_foc_and_ws_van_foc")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + ".process_vanadium")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_save_directories_both_banks_with_RBnum(
        self,
        mock_load_inst_cal,
        mock_proc_van,
        mock_load_run,
        mock_foc_run,
        mock_check_foc_and_van_foc,
        mock_apply_van,
        mock_save_out,
        mock_conv_units,
        mock_del_ws,
    ):
        rb_num = "1"
        van_run = "123456"
        mock_proc_van.return_value = ("van_ws_foc", van_run)
        mock_load_run.return_value = MagicMock()
        sample_foc_ws = MagicMock()
        sample_foc_ws.name.return_value = "foc_name"
        mock_apply_van.return_value = sample_foc_ws  # xunit = dSpacing
        mock_conv_units.return_value = sample_foc_ws  # xunit = TOF
        self.calibration.group = GROUP.BOTH
        mock_save_out.return_value = ["Nexus files"], ["GSS files"]

        # plotting focused runs
        self.model.focus_run(["305761"], "fake/van/path", plot_output=False, rb_num=rb_num, calibration=self.calibration, save_dir="dir")

        self.assertEqual(mock_save_out.call_count, 2)  # once for dSpacing and once for TOF
        save_calls = 2 * [
            call([path.join("dir", "Focus"), path.join("dir", "User", rb_num, "Focus")], sample_foc_ws, self.calibration, van_run, rb_num)
        ]
        mock_save_out.assert_has_calls(save_calls)

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + ".mantid.ConvertUnits")
    @patch(enggutils_path + "._save_output_files")
    @patch(enggutils_path + "._apply_vanadium_norm")
    @patch(enggutils_path + "._check_ws_foc_and_ws_van_foc")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + ".process_vanadium")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_save_directories_texture_with_RBnum(
        self,
        mock_load_inst_cal,
        mock_proc_van,
        mock_load_run,
        mock_foc_run,
        mock_check_foc_and_van_foc,
        mock_apply_van,
        mock_save_out,
        mock_conv_units,
        mock_del_ws,
    ):
        rb_num = "1"
        van_run = "123456"
        mock_proc_van.return_value = ("van_ws_foc", van_run)
        mock_load_run.return_value = MagicMock()
        sample_foc_ws = MagicMock()
        sample_foc_ws.name.return_value = "foc_name"
        mock_apply_van.return_value = sample_foc_ws  # xunit = dSpacing
        mock_conv_units.return_value = sample_foc_ws  # xunit = TOF
        self.calibration.group = GROUP.TEXTURE20
        mock_save_out.return_value = ["Nexus files"], ["GSS files"]

        # plotting focused runs
        self.model.focus_run(["305761"], "fake/van/path", plot_output=False, rb_num=rb_num, calibration=self.calibration, save_dir="dir")

        self.assertEqual(mock_save_out.call_count, 2)  # once for dSpacing and once for TOF
        save_calls = 2 * [call([path.join("dir", "User", rb_num, "Focus")], sample_foc_ws, self.calibration, van_run, rb_num)]
        mock_save_out.assert_has_calls(save_calls)

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + ".mantid.ConvertUnits")
    @patch(enggutils_path + "._save_output_files")
    @patch(enggutils_path + "._apply_vanadium_norm")
    @patch(enggutils_path + "._check_ws_foc_and_ws_van_foc")
    @patch(enggutils_path + "._focus_run_and_apply_roi_calibration")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + ".process_vanadium")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_save_directories_texture30_with_RBnum(
        self,
        mock_load_inst_cal,
        mock_proc_van,
        mock_load_run,
        mock_foc_run,
        mock_check_foc_and_van_foc,
        mock_apply_van,
        mock_save_out,
        mock_conv_units,
        mock_del_ws,
    ):
        rb_num = "1"
        van_run = "123456"
        mock_proc_van.return_value = ("van_ws_foc", van_run)
        mock_load_run.return_value = MagicMock()
        sample_foc_ws = MagicMock()
        sample_foc_ws.name.return_value = "foc_name"
        mock_apply_van.return_value = sample_foc_ws  # xunit = dSpacing
        mock_conv_units.return_value = sample_foc_ws  # xunit = TOF
        self.calibration.group = GROUP.TEXTURE30
        mock_save_out.return_value = ["Nexus files"], ["GSS files"]

        # plotting focused runs
        self.model.focus_run(["305761"], "fake/van/path", plot_output=False, rb_num=rb_num, calibration=self.calibration, save_dir="dir")

        self.assertEqual(mock_save_out.call_count, 2)  # once for dSpacing and once for TOF
        save_calls = 2 * [call([path.join("dir", "User", rb_num, "Focus")], sample_foc_ws, self.calibration, van_run, rb_num)]
        mock_save_out.assert_has_calls(save_calls)

    @patch(enggutils_path + ".mantid.DeleteWorkspace")
    @patch(enggutils_path + "._save_output_files")
    @patch(enggutils_path + "._load_run_and_convert_to_dSpacing")
    @patch(enggutils_path + "._plot_focused_workspaces")
    @patch(enggutils_path + ".process_vanadium")
    @patch(file_path + ".load_full_instrument_calibration")
    def test_focus_run_when_zero_proton_charge(
        self, mock_load_inst_cal, mock_proc_van, mock_plot, mock_load_run, mock_save_out, mock_del_ws
    ):
        mock_proc_van.return_value = ("van_ws_foc", "123456")
        mock_load_run.return_value = None  # expected return when no proton charge

        self.model.focus_run(["305761"], "fake/van/path", plot_output=True, rb_num=None, calibration=self.calibration, save_dir="dir")

        mock_plot.assert_not_called()
        mock_save_out.assert_not_called()
        mock_del_ws.assert_not_called()


if __name__ == "__main__":
    unittest.main()
