# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import unittest
from os import path

from unittest import mock
from unittest.mock import patch, MagicMock
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings import settings_presenter

dir_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_presenter"


class SettingsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.model = MagicMock()
        self.model.validate_gsas2_path.return_value = (True, "")
        self.view = MagicMock()
        self.view.set_euler_options_enabled = MagicMock()
        self.view.set_contour_option_enabled = MagicMock()
        self.presenter = settings_presenter.SettingsPresenter(self.model, self.view)
        self.presenter.settings = {}
        self.settings = {
            "clear_absorption_ws_after_processing": True,
            "contour_kernel": "2.0",
            "cost_func_thresh": "100",
            "dSpacing_min": 1.0,
            "default_peak": "BackToBackExponential",
            "euler_angles_scheme": "YZY",
            "euler_angles_sense": "1,-1,1",
            "full_calibration": "cal",
            "logs": "some,logs",
            "monte_carlo_params": "SparseInstrument:True",
            "nd_dir": "0,1,0",
            "nd_name": "ND",
            "path_to_gsas2": "/opt/gsas2/",
            "peak_pos_thresh": "0.1",
            "plot_exp_pf": True,
            "primary_log": "some",
            "rd_dir": "1,0,0",
            "rd_name": "RD",
            "save_location": "save",
            "sort_ascending": True,
            "td_dir": "0,0,1",
            "td_name": "TD",
            "timeout": 10,
            "use_euler_angles": False,
            "auto_pop_texture": False,
        }

    def setup_view_getters(self, blank_log=False):
        self.view.get_save_location.return_value = self.settings["save_location"][:]
        self.view.get_rd_name.return_value = self.settings["rd_name"]
        self.view.get_nd_name.return_value = self.settings["nd_name"]
        self.view.get_td_name.return_value = self.settings["td_name"]
        self.view.get_rd_dir.return_value = self.settings["rd_dir"]
        self.view.get_nd_dir.return_value = self.settings["nd_dir"]
        self.view.get_td_dir.return_value = self.settings["td_dir"]
        self.view.get_full_calibration.return_value = self.settings["full_calibration"][:]
        self.view.get_checked_logs.return_value = self.settings["logs"][:]
        self.view.get_primary_log.return_value = self.settings["primary_log"] if not blank_log else ""
        self.view.get_ascending_checked.return_value = self.settings["sort_ascending"]
        self.view.get_peak_function.return_value = self.settings["default_peak"]
        self.view.get_path_to_gsas2.return_value = self.settings["path_to_gsas2"]
        self.view.get_timeout.return_value = self.settings["timeout"]
        self.view.get_dSpacing_min.return_value = self.settings["dSpacing_min"]
        self.view.get_monte_carlo_params.return_value = self.settings["monte_carlo_params"]
        self.view.get_remove_corr_ws_after_processing.return_value = self.settings["clear_absorption_ws_after_processing"]
        self.view.get_cost_func_thresh.return_value = self.settings["cost_func_thresh"]
        self.view.get_peak_pos_thresh.return_value = self.settings["peak_pos_thresh"]
        self.view.get_use_euler_angles.return_value = self.settings["use_euler_angles"]
        self.view.get_euler_angles_scheme.return_value = self.settings["euler_angles_scheme"]
        self.view.get_euler_angles_sense.return_value = self.settings["euler_angles_sense"]
        self.view.get_plot_exp_pf.return_value = self.settings["plot_exp_pf"]
        self.view.get_contour_kernel.return_value = self.settings["contour_kernel"]
        self.view.get_auto_populate_texture.return_value = self.settings["auto_pop_texture"]

    @patch(dir_path + ".path.isfile")
    def test_load_existing_settings(self, mock_isfile):
        mock_isfile.return_value = True
        self.model.get_settings_dict.return_value = self.settings.copy()
        self.model.validate_settings.return_value = self.settings.copy()

        self.presenter.load_settings_from_file_or_default()

        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_not_called()

    def test_file_searched_on_opening(self):
        self.model.get_settings_dict.return_value = self.settings.copy()
        self.model.validate_settings.return_value = self.settings.copy()

        self.presenter.load_settings_from_file_or_default()

        self.assertEqual(1, self.view.find_full_calibration.call_count)
        self.assertEqual(1, self.view.find_save.call_count)

    @patch(dir_path + ".path.isfile")
    def test_save_new_settings(self, mock_isfile):
        mock_isfile.return_value = True
        self.setup_view_getters()
        self.model.validate_settings.return_value = self.settings.copy()
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.save_new_settings()

        self.assertEqual(self.view.close.call_count, 0)
        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_called_with(self.settings)
        self.assertEqual(self.presenter.savedir_notifier.notify_subscribers.call_count, 1)

    @patch(dir_path + ".path.isfile")
    def test_show(self, mock_isfile):
        mock_isfile.return_value = True
        self.presenter.settings = self.settings.copy()
        self.model.validate_settings.return_value = self.settings.copy()

        self.presenter.show()

        # check that view is updated before being shown
        self.view.set_save_location.assert_called_with(self.settings["save_location"])
        self.view.set_full_calibration.assert_called_with(self.settings["full_calibration"])
        self.view.set_checked_logs.assert_called_with(self.settings["logs"])
        self.view.set_primary_log_combobox.assert_called_with(self.settings["primary_log"])
        self.view.set_ascending_checked.assert_called_with(self.settings["sort_ascending"])
        self.view.set_peak_function.assert_called_with(self.settings["default_peak"])

    @patch(dir_path + ".path.isfile")
    def test_save_settings_and_close(self, mock_isfile):
        mock_isfile.return_value = True
        self.setup_view_getters()
        self.model.validate_settings.return_value = self.settings.copy()
        self.presenter.savedir_notifier = mock.MagicMock()

        self.presenter.save_and_close_dialog()

        self.assertEqual(self.presenter.settings, self.settings)
        self.model.set_settings_dict.assert_called_with(self.settings)
        self.assertEqual(self.view.close.call_count, 1)
        self.assertEqual(self.presenter.savedir_notifier.notify_subscribers.call_count, 1)

    def test_settings_not_changed_when_cancelled(self):
        self.presenter.close_dialog()
        self.model.set_settings_dict.assert_not_called()

    def test_default_calib_file_correct_location(self):
        self.assertTrue(path.exists(settings_presenter.DEFAULT_SETTINGS["full_calibration"]))


if __name__ == "__main__":
    unittest.main()
