# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

import unittest

from mantid.py3compat.mock import patch, MagicMock
from mantid.py3compat import mock
from Engineering.gui.engineering_diffraction.tabs.calibration import model, view, presenter

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

    @patch(tab_path + ".presenter.CalibrationPresenter._create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_while_finder_is_searching(self, worker_method, err_msg):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = True

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    @patch(tab_path + ".presenter.CalibrationPresenter._create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_run_numbers")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_run_numbers_invalid(self, worker_method, validator, err_msg):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        validator.return_value = False

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    def test_controls_disabled_disables_both(self):
        self.presenter.set_calibrate_controls_enabled(False)

        self.view.set_calibrate_button_enabled.assert_called_with(False)
        self.view.set_check_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.set_calibrate_controls_enabled(True)

        self.view.set_calibrate_button_enabled.assert_called_with(True)
        self.view.set_check_plot_output_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.CalibrationPresenter.emit_enable_button_signal")
    @patch(tab_path + ".presenter.logger.warning")
    def test_on_error_posts_to_logger_and_enables_controls(self, logger, emit):
        fail_info = 2024278

        self.presenter._on_error(fail_info)

        logger.assert_called_with(str(fail_info))
        self.assertEqual(emit.call_count, 1)

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

    def test_set_instrument_override_ENGINX(self):
        instrument = 0
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with("ENGINX")
        self.assertEqual(self.presenter.instrument, "ENGINX")

    def test_set_instrument_override_IMAT(self):
        instrument = 1
        self.presenter.set_instrument_override(instrument)

        self.view.set_instrument_override.assert_called_with("IMAT")
        self.assertEqual(self.presenter.instrument, "IMAT")

    def test_set_current_calibration(self):
        self.presenter.calibration_notifier = MagicMock()
        pending = {
            "vanadium_path": "/test/set/path",
            "ceria_path": "test/set/path/2",
            "instrument": "TEST_INS"
        }
        self.presenter.pending_calibration = pending
        current = {
            "vanadium_path": "old/value",
            "ceria_path": "old/cera",
            "instrument": "ENGINX"
        }
        blank = {
            "vanadium_path": None,
            "ceria_path": None,
            "instrument": None
        }
        self.presenter.current_calibration = current

        self.assertEqual(self.presenter.current_calibration, current)
        self.presenter.set_current_calibration()

        self.assertEqual(self.presenter.current_calibration, pending)
        self.assertEqual(self.presenter.pending_calibration, blank)
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 1)
        self.assertEqual(self.view.set_vanadium_text.call_count, 1)
        self.assertEqual(self.view.set_calib_text.call_count, 1)

    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_valid_path(self, path):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True
        path.return_value = True
        instrument, van, cer = ("test_ins", "test_van", "test_cer")
        self.model.load_existing_gsas_parameters.return_value = instrument, van, cer
        current = {
            "vanadium_path": "old/value",
            "ceria_path": "old/cera",
            "instrument": "ENGINX"
        }
        new = {
            "vanadium_path": van,
            "ceria_path": cer,
            "instrument": instrument
        }
        self.presenter.current_calibration = current

        self.presenter.on_calibrate_clicked()

        self.assertEqual(self.presenter.current_calibration, new)
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 1)

    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_invalid_path(self, path):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True
        path.return_value = False
        current = {
            "vanadium_path": "old/value",
            "ceria_path": "old/cera",
            "instrument": "ENGINX"
        }
        self.presenter.current_calibration = current

        self.presenter.on_calibrate_clicked()

        self.assertEqual(self.presenter.current_calibration, current)
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 0)

    def test_create_new_enabled_true(self):
        self.presenter.set_create_new_enabled(True)

        self.assertEqual(self.view.set_vanadium_enabled.call_count, 1)
        self.view.set_vanadium_enabled.assert_called_with(True)
        self.assertEqual(self.view.set_calib_enabled.call_count, 1)
        self.view.set_calib_enabled.assert_called_with(True)
        self.view.set_calibrate_button_text.assert_called_with("Calibrate")
        self.view.set_check_plot_output_enabled.assert_called_with(True)
        self.assertEqual(self.view.find_calib_files.call_count, 1)

    def test_create_new_enabled_false(self):
        self.presenter.set_create_new_enabled(False)

        self.assertEqual(self.view.set_vanadium_enabled.call_count, 1)
        self.view.set_vanadium_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calib_enabled.call_count, 1)
        self.view.set_calib_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calibrate_button_text.call_count, 0)
        self.assertEqual(self.view.set_check_plot_output_enabled.call_count, 0)
        self.assertEqual(self.view.find_calib_files.call_count, 0)

    def test_load_existing_enabled_true(self):
        self.presenter.set_load_existing_enabled(True)

        self.assertEqual(self.view.set_path_enabled.call_count, 1)
        self.view.set_path_enabled.assert_called_with(True)
        self.view.set_calibrate_button_text.assert_called_with("Load")
        self.view.set_check_plot_output_enabled.assert_called_with(False)

    def test_load_existing_enabled_false(self):
        self.presenter.set_load_existing_enabled(False)

        self.assertEqual(self.view.set_path_enabled.call_count, 1)
        self.view.set_path_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calibrate_button_text.call_count, 0)
        self.assertEqual(self.view.set_check_plot_output_enabled.call_count, 0)

    @patch(tab_path + ".presenter.AsyncTask")
    def test_start_calibration_worker(self, task):
        instrument, van, cer = ("test_ins", "test_van", "test_cer")
        old_pending = {
            "vanadium_path": None,
            "ceria_path": None,
            "instrument": None
        }
        self.presenter.pending_calibration = old_pending
        expected_pending = {
            "vanadium_path": van,
            "ceria_path": cer,
            "instrument": instrument
        }
        self.presenter.instrument = instrument

        self.presenter.start_calibration_worker(van, cer, False, None)

        self.assertEqual(self.presenter.pending_calibration, expected_pending)


if __name__ == '__main__':
    unittest.main()
