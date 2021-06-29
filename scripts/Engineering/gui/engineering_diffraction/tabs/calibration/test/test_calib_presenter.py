# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, MagicMock
from unittest import mock
from Engineering.gui.engineering_diffraction.tabs.calibration import model, view, presenter
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

tab_path = 'Engineering.gui.engineering_diffraction.tabs.calibration'


class CalibrationPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.CalibrationView)
        self.model = mock.create_autospec(model.CalibrationModel)
        self.view.get_cropping_widget.return_value = MagicMock()
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)
        self.presenter.cropping_widget = MagicMock()

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_started_with_right_params(self, worker_method):
        self.view.get_crop_checked.return_value = False
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False

        self.presenter.on_calibrate_clicked()
        worker_method.assert_called_with("307521", "305738", True, None)

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_started_with_right_params_crop_bank(self, worker_method):
        self.view.get_crop_checked.return_value = True
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.presenter.cropping_widget.is_custom.return_value = False
        self.presenter.cropping_widget.get_bank.return_value = "bank"

        self.presenter.on_calibrate_clicked()
        worker_method.assert_called_with("307521", "305738", True, None, bank="bank")

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_started_with_right_params_crop_spec_nums(self, worker_method):
        self.view.get_crop_checked.return_value = True
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.presenter.cropping_widget.is_custom.return_value = True
        self.presenter.cropping_widget.get_custom_spectra.return_value = "1-56,401-809"

        self.presenter.on_calibrate_clicked()
        worker_method.assert_called_with("307521", "305738", True, None, spectrum_numbers="1-56,401-809")

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_while_finder_is_searching(self, worker_method, err_msg):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.get_load_checked.return_value = False
        self.view.is_searching.return_value = True

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_run_numbers")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_run_numbers_invalid(self, worker_method, validator, err_msg):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.view.get_load_checked.return_value = False
        validator.return_value = False

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_run_numbers")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_cropping_invalid(self, worker_method, validator, err_msg):
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.view.get_load_checked.return_value = False
        validator.return_value = True
        self.presenter.cropping_widget.is_valid.return_value = False

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
    def test_on_error_posts_to_logger_and_enables_controls(self, emit):
        fail_info = 2024278

        self.presenter._on_error(fail_info)

        self.assertEqual(emit.call_count, 1)

    def test_validation_of_run_numbers(self):
        self.view.get_sample_valid.return_value = False
        self.view.get_vanadium_valid.return_value = False
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_sample_valid.return_value = True
        self.view.get_vanadium_valid.return_value = False
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_sample_valid.return_value = False
        self.view.get_vanadium_valid.return_value = True
        result = self.presenter.validate_run_numbers()
        self.assertFalse(result)

        self.view.get_sample_valid.return_value = True
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

    @patch(tab_path + ".presenter.CalibrationPresenter.emit_update_fields_signal")
    def test_set_current_calibration(self, update_sig):
        self.presenter.calibration_notifier = MagicMock()
        pending = CalibrationInfo(vanadium_path="/test/set/path",
                                  sample_path="test/set/path/2",
                                  instrument="TEST_INS")
        pendcpy = CalibrationInfo(vanadium_path="/test/set/path",
                                  sample_path="test/set/path/2",
                                  instrument="TEST_INS")
        self.presenter.pending_calibration = pending
        current = CalibrationInfo(vanadium_path="old/value",
                                  sample_path="old/cera",
                                  instrument="ENGINX")
        blank = CalibrationInfo()
        self.presenter.current_calibration = current

        self.presenter.set_current_calibration()

        self.check_calibration_equal(self.presenter.current_calibration, pendcpy)
        self.check_calibration_equal(self.presenter.pending_calibration, blank)
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 1)
        self.assertEqual(update_sig.call_count, 1)

    @patch(tab_path + ".presenter.CalibrationPresenter.emit_update_fields_signal")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_valid_path(self, path, update):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True

        path.return_value = True
        instrument, van, cer = ("test_ins", "test_van", "test_cer")
        self.model.load_existing_gsas_parameters.return_value = instrument, van, cer
        current = CalibrationInfo(vanadium_path="old/value",
                                  sample_path="old/cera",
                                  instrument="ENGINX")
        new = CalibrationInfo(vanadium_path=van, sample_path=cer, instrument=instrument)
        self.presenter.current_calibration = current

        self.presenter.on_calibrate_clicked()

        self.assertEqual(update.call_count, 1)
        self.assertEqual(self.presenter.current_calibration.get_vanadium(), new.get_vanadium())
        self.assertEqual(self.presenter.current_calibration.get_sample(), new.get_sample())
        self.assertEqual(self.presenter.current_calibration.get_instrument(), new.get_instrument())
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 1)

    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_invalid_path(self, path):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True
        path.return_value = False
        current = CalibrationInfo(vanadium_path="old/value",
                                  sample_path="old/cera",
                                  instrument="ENGINX")
        self.presenter.current_calibration = current

        self.presenter.on_calibrate_clicked()

        self.assertEqual(self.presenter.current_calibration, current)
        self.assertEqual(self.presenter.calibration_notifier.notify_subscribers.call_count, 0)

    def test_create_new_enabled_true(self):
        self.presenter.set_create_new_enabled(True)

        self.assertEqual(self.view.set_vanadium_enabled.call_count, 1)
        self.view.set_vanadium_enabled.assert_called_with(True)
        self.assertEqual(self.view.set_sample_enabled.call_count, 1)
        self.view.set_sample_enabled.assert_called_with(True)
        self.view.set_calibrate_button_text.assert_called_with("Calibrate")
        self.view.set_check_plot_output_enabled.assert_called_with(True)
        self.assertEqual(self.view.find_sample_files.call_count, 1)

    def test_create_new_enabled_false(self):
        self.presenter.set_create_new_enabled(False)

        self.assertEqual(self.view.set_vanadium_enabled.call_count, 1)
        self.view.set_vanadium_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_sample_enabled.call_count, 1)
        self.view.set_sample_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calibrate_button_text.call_count, 0)
        self.assertEqual(self.view.set_check_plot_output_enabled.call_count, 0)
        self.assertEqual(self.view.find_sample_files.call_count, 0)

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
        old_pending = CalibrationInfo(vanadium_path=None, sample_path=None, instrument=None)
        self.presenter.pending_calibration = old_pending
        expected_pending = CalibrationInfo(vanadium_path=van,
                                           sample_path=cer,
                                           instrument=instrument)
        self.presenter.instrument = instrument

        self.presenter.start_calibration_worker(van, cer, False, None)

        self.check_calibration_equal(self.presenter.pending_calibration, expected_pending)

    def test_cropping_disabled_when_loading_calib(self):
        self.presenter.set_load_existing_enabled(True)

        self.view.set_cropping_widget_visibility.assert_called_with(False)
        self.view.set_check_cropping_enabled.assert_called_with(False)
        self.view.set_check_cropping_checked.assert_called_with(False)

    def check_calibration_equal(self, a, b):
        self.assertEqual(a.get_vanadium(), b.get_vanadium())
        self.assertEqual(a.get_sample(), b.get_sample())
        self.assertEqual(a.get_instrument(), b.get_instrument())


if __name__ == '__main__':
    unittest.main()
