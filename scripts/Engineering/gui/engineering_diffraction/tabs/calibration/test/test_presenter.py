# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import patch
from mantid.py3compat import mock
from Engineering.gui.engineering_diffraction.tabs.calibration import view, model, presenter

tab_path = 'Engineering.gui.engineering_diffraction.tabs.calibration'


class CalibrationPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.CalibrationView)
        self.model = mock.create_autospec(model.CalibrationModel)
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_started_with_right_params(self, worker_method):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False

        self.presenter.on_calibrate_clicked()
        worker_method.assert_called_with("307521", "305738", True, None)

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_while_finder_is_searching(self, worker_method):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = True

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()

    @patch(tab_path + ".presenter.CalibrationPresenter.validate_run_numbers")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_run_numbers_invalid(self, worker_method, validator):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        validator.return_value = False

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()

    def test_controls_disabled_disables_both(self):
        self.presenter.disable_calibrate_buttons()

        self.view.set_calibrate_button_enabled.assert_called_with(False)
        self.view.set_check_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.disable_calibrate_buttons()

        self.view.set_calibrate_button_enabled.assert_called_with(False)
        self.view.set_check_plot_output_enabled.assert_called_with(False)

        self.presenter.enable_calibrate_buttons()

        self.view.set_calibrate_button_enabled.assert_called_with(True)
        self.view.set_check_plot_output_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.logger.warning")
    def test_on_error_posts_to_logger_and_enables_controls(self, logger):
        fail_info = 2024278

        self.presenter._on_error(fail_info)

        logger.assert_called_with(str(fail_info))
        self.view.set_calibrate_button_enabled.assert_called_with(True)
        self.view.set_check_plot_output_enabled.assert_called_with(True)

    def test_validation_of_run_numbers(self):
        self.view.get_calib_valid.return_value = False
        self.view.get_vanadium_valid.return_value = False
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_calib_valid.return_value = True
        self.view.get_vanadium_valid.return_value = False
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_calib_valid.return_value = False
        self.view.get_vanadium_valid.return_value = True
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_calib_valid.return_value = True
        self.view.get_vanadium_valid.return_value = True
        result = self.presenter.validate_run_numbers()
        self.assertTrue(result)

    def test_set_instrument_override(self):
        instrument = "TEST"
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with(instrument)
        self.assertEqual(self.presenter.instrument, instrument)
