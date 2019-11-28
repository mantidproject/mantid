# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat import mock
from mantid.py3compat.mock import patch
from Engineering.gui.engineering_diffraction.tabs.focus import model, view, presenter
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

tab_path = "Engineering.gui.engineering_diffraction.tabs.focus"


class FocusPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FocusView)
        self.model = mock.create_autospec(model.FocusModel)
        self.presenter = presenter.FocusPresenter(self.model, self.view)

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params(self, worker, wsp_exists):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.get_focus_filename.return_value = "305738"
        self.view.get_north_bank.return_value = False
        self.view.get_south_bank.return_value = True
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        wsp_exists.return_value = True

        self.presenter.on_focus_clicked()
        worker.assert_called_with("305738", ["South"], True, None)

    @patch(tab_path + ".presenter.FocusPresenter._validate")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_not_started_validate_fails(self, worker, valid):
        valid.return_value = False

        self.presenter.on_focus_clicked()
        worker.assert_not_called()

    def test_controls_disabled_disables_both(self):
        self.presenter.set_focus_controls_enabled(False)

        self.view.set_focus_button_enabled.assert_called_with(False)
        self.view.set_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.set_focus_controls_enabled(True)

        self.view.set_focus_button_enabled.assert_called_with(True)
        self.view.set_plot_output_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.FocusPresenter.emit_enable_button_signal")
    @patch(tab_path + ".presenter.logger.warning")
    def test_on_worker_error_posts_to_logger_and_enables_controls(self, logger, emit):
        fail_info = 2024278

        self.presenter._on_worker_error(fail_info)

        logger.assert_called_with(str(fail_info))
        self.assertEqual(emit.call_count, 1)

    def test_get_both_banks(self):
        self.view.get_north_bank.return_value = True
        self.view.get_south_bank.return_value = True

        self.assertEqual(["North", "South"], self.presenter._get_banks())

    def test_get_north_bank(self):
        self.view.get_north_bank.return_value = True
        self.view.get_south_bank.return_value = False

        self.assertEqual(["North"], self.presenter._get_banks())

    def test_get_south_bank(self):
        self.view.get_north_bank.return_value = False
        self.view.get_south_bank.return_value = True

        self.assertEqual(["South"], self.presenter._get_banks())

    def test_getting_no_banks(self):
        self.view.get_north_bank.return_value = False
        self.view.get_south_bank.return_value = False

        self.assertEqual([], self.presenter._get_banks())

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_focus_path(self, error_message):
        self.view.get_focus_valid.return_value = False
        banks = ["North", "South"]

        self.assertFalse(self.presenter._validate(banks))
        self.assertEqual(error_message.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_calibration(self, create_error):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path=None,
                                                             sample_path=None,
                                                             instrument=None)
        banks = ["North", "South"]

        self.presenter._validate(banks)
        create_error.assert_called_with(
            self.presenter.view, "Create or Load a calibration via the Calibration tab before focusing.")

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_while_searching(self, create_error, wsp_check):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/File/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.is_searching.return_value = True
        wsp_check.return_value = True
        banks = ["North", "South"]

        self.assertEqual(False, self.presenter._validate(banks))
        self.assertEqual(1, create_error.call_count)

    @patch(tab_path + ".presenter.check_workspaces_exist")
    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_no_banks_selected(self, create_error, wsp_check):
        self.presenter.current_calibration = CalibrationInfo(vanadium_path="Fake/Path",
                                                             sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.is_searching.return_value = False
        banks = []
        wsp_check.return_value = True

        self.presenter._validate(banks)
        create_error.assert_called_with(self.presenter.view, "Please select at least one bank.")


if __name__ == '__main__':
    unittest.main()
