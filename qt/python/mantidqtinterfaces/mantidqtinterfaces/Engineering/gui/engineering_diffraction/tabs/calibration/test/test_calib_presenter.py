# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import patch, MagicMock
from unittest import mock
from Engineering.EnggUtils import GROUP
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration import model, view, presenter
from Engineering.common.calibration_info import CalibrationInfo

tab_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.calibration"


class CalibrationPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.CalibrationView, instance=True)
        self.model = mock.create_autospec(model.CalibrationModel, instance=True)
        self.view.get_cropping_widget.return_value = MagicMock()
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)
        self.presenter.cropping_widget = MagicMock()

    def test_update_calibration_from_view_no_cropping(self):
        self.view.get_load_checked.return_value = False
        self.view.get_crop_checked.return_value = False
        self.presenter.instrument = "ENGINX"
        self.view.get_sample_filename.return_value = "193749"
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.update_calibration_from_view()

        self.presenter.current_calibration.set_calibration_paths.assert_called_once_with("ENGINX", "193749")
        self.presenter.current_calibration.set_group.assert_called_once_with(GROUP.BOTH)
        self.presenter.current_calibration.set_grouping_file.assert_not_called()
        self.presenter.current_calibration.set_spectra_list.assert_not_called()

    def test_update_calibration_from_view_cropped_to_bank(self):
        self.view.get_load_checked.return_value = False
        self.view.get_crop_checked.return_value = True
        self.presenter.cropping_widget.get_group.return_value = GROUP.NORTH
        self.presenter.instrument = "ENGINX"
        self.view.get_sample_filename.return_value = "193749"
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.update_calibration_from_view()

        self.presenter.current_calibration.set_calibration_paths.assert_called_once_with("ENGINX", "193749")
        self.presenter.current_calibration.set_group.assert_called_once_with(self.presenter.cropping_widget.get_group())
        self.presenter.current_calibration.set_grouping_file.assert_not_called()
        self.presenter.current_calibration.set_spectra_list.assert_not_called()

    def test_update_calibration_from_view_cropped_to_texture30(self):
        self.view.get_load_checked.return_value = False
        self.view.get_crop_checked.return_value = True
        self.presenter.cropping_widget.get_group.return_value = GROUP.TEXTURE30
        self.presenter.instrument = "ENGINX"
        self.view.get_sample_filename.return_value = "193749"
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.update_calibration_from_view()

        self.presenter.current_calibration.set_calibration_paths.assert_called_once_with("ENGINX", "193749")
        self.presenter.current_calibration.set_group.assert_called_once_with(self.presenter.cropping_widget.get_group())
        self.presenter.current_calibration.set_grouping_file.assert_not_called()
        self.presenter.current_calibration.set_spectra_list.assert_not_called()

    def test_update_calibration_from_view_cropped_to_spectra(self):
        self.view.get_load_checked.return_value = False
        self.view.get_crop_checked.return_value = True
        self.presenter.cropping_widget.get_group.return_value = GROUP.CROPPED
        self.presenter.cropping_widget.get_custom_spectra.return_value = "1"
        self.presenter.instrument = "ENGINX"
        self.view.get_sample_filename.return_value = "193749"
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)
        self.presenter.current_calibration.group = self.presenter.cropping_widget.get_group()

        self.presenter.update_calibration_from_view()

        self.presenter.current_calibration.set_calibration_paths.assert_called_once_with("ENGINX", "193749")
        self.presenter.current_calibration.set_group.assert_called_once_with(self.presenter.cropping_widget.get_group())
        self.presenter.current_calibration.set_grouping_file.assert_not_called()
        self.presenter.current_calibration.set_spectra_list.assert_called_once_with(self.presenter.cropping_widget.get_custom_spectra())

    def test_update_calibration_from_view_custom_calfile(self):
        self.view.get_load_checked.return_value = False
        self.view.get_crop_checked.return_value = True
        self.presenter.cropping_widget.get_group.return_value = GROUP.CUSTOM
        self.presenter.cropping_widget.get_custom_groupingfile.return_value = "cal"
        self.presenter.instrument = "ENGINX"
        self.view.get_sample_filename.return_value = "193749"
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)
        self.presenter.current_calibration.group = self.presenter.cropping_widget.get_group()

        self.presenter.update_calibration_from_view()

        self.presenter.current_calibration.set_calibration_paths.assert_called_once_with("ENGINX", "193749")
        self.presenter.current_calibration.set_group.assert_called_once_with(self.presenter.cropping_widget.get_group())
        self.presenter.current_calibration.set_grouping_file.assert_called_once_with(
            self.presenter.cropping_widget.get_custom_groupingfile()
        )
        self.presenter.current_calibration.set_spectra_list.assert_not_called()

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_while_finder_is_searching(self, worker_method, err_msg):
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.get_load_checked.return_value = False
        self.view.is_searching.return_value = True

        self.presenter.on_calibrate_clicked()
        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_run_numbers_invalid(self, worker_method, err_msg):
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.view.get_load_checked.return_value = False
        self.view.get_sample_valid.return_value = False

        self.presenter.on_calibrate_clicked()

        worker_method.assert_not_called()
        self.assertEqual(err_msg.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_not_started_when_cropping_invalid(self, worker_method, err_msg):
        self.view.get_sample_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        self.view.get_new_checked.return_value = True
        self.view.get_load_checked.return_value = False
        self.presenter.cropping_widget.is_spectra_valid.return_value = False

        self.presenter.on_calibrate_clicked()

        self.model.load_existing_calibration_files.assert_not_called()  # called if loaded calibration
        worker_method.assert_not_called()  # called if created new one
        self.assertEqual(err_msg.call_count, 1)

    def test_controls_disabled_disables_both(self):
        self.presenter.set_calibrate_controls_enabled(False)

        self.view.set_calibrate_button_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.set_calibrate_controls_enabled(True)

        self.view.set_calibrate_button_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.CalibrationPresenter.emit_enable_button_signal")
    def test_on_error_posts_to_logger_and_enables_controls(self, emit_button):
        fail_info = 2024278

        self.presenter._on_error(fail_info)

        self.assertEqual(emit_button.call_count, 1)

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

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_valid_path(self, mock_valid_path, mock_setting):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True
        prm_filepath = "path/to/prm.prm"
        self.view.get_path_filename.return_value = prm_filepath
        mock_valid_path.return_value = True
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.on_calibrate_clicked()

        self.model.load_existing_calibration_files.assert_called_with(self.presenter.current_calibration)
        self.presenter.current_calibration.set_calibration_from_prm_fname.assert_called_with(prm_filepath)
        self.presenter.calibration_notifier.notify_subscribers.assert_called_once()
        mock_setting.assert_called_once()

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.CalibrationPresenter.validate_path")
    def test_calibrate_clicked_load_invalid_path(self, mock_valid_path, mock_setting):
        self.presenter.calibration_notifier = MagicMock()
        self.view.get_new_checked.return_value = False
        self.view.get_load_checked.return_value = True
        mock_valid_path.return_value = False
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.on_calibrate_clicked()

        self.presenter.current_calibration.set_calibration_from_prm_fname.assert_not_called()
        self.presenter.calibration_notifier.notify_subscribers.assert_not_called()
        mock_setting.assert_not_called()

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    @patch(tab_path + ".presenter.CalibrationPresenter._validate")
    def test_calibrate_clicked_new_valid_calibration_with_plotting(self, mock_validate, mock_start_worker):
        self.view.get_new_checked.return_value = True
        self.view.get_plot_output.return_value = True
        mock_validate.return_value = True
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.on_calibrate_clicked()

        mock_start_worker.assert_called_once_with(mock_validate())

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    @patch(tab_path + ".presenter.CalibrationPresenter._validate")
    def test_calibrate_clicked_new_invalid_calibration(self, mock_validate, mock_start_worker):
        self.view.get_new_checked.return_value = True
        self.view.get_plot_output.return_value = True
        self.view.get_load_checked.return_value = False
        mock_validate.return_value = False
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo(), instance=True)

        self.presenter.on_calibrate_clicked()

        mock_start_worker.assert_not_called()  # called if created new one
        self.model.load_existing_calibration_files.assert_not_called()  # called if loaded calibration

    def test_create_new_enabled_true(self):
        self.presenter.set_create_new_enabled(True)

        self.assertEqual(self.view.set_sample_enabled.call_count, 1)
        self.view.set_sample_enabled.assert_called_with(True)
        self.view.set_calibrate_button_text.assert_called_with("Calibrate")
        self.assertEqual(self.view.find_sample_files.call_count, 1)

    def test_create_new_enabled_false(self):
        self.presenter.set_create_new_enabled(False)

        self.assertEqual(self.view.set_sample_enabled.call_count, 1)
        self.view.set_sample_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calibrate_button_text.call_count, 0)
        self.assertEqual(self.view.find_sample_files.call_count, 0)

    def test_load_existing_enabled_true(self):
        self.presenter.set_load_existing_enabled(True)

        self.assertEqual(self.view.set_path_enabled.call_count, 1)
        self.view.set_path_enabled.assert_called_with(True)
        self.view.set_calibrate_button_text.assert_called_with("Load")

    def test_load_existing_enabled_false(self):
        self.presenter.set_load_existing_enabled(False)

        self.assertEqual(self.view.set_path_enabled.call_count, 1)
        self.view.set_path_enabled.assert_called_with(False)
        self.assertEqual(self.view.set_calibrate_button_text.call_count, 0)

    def test_cropping_disabled_when_loading_calib(self):
        self.presenter.set_load_existing_enabled(True)

        self.view.set_cropping_widget_visibility.assert_called_with(False)
        self.view.set_check_cropping_enabled.assert_called_with(False)
        self.view.set_check_cropping_checked.assert_called_with(False)

    def check_calibration_equal(self, a, b):
        self.assertEqual(a.get_sample(), b.get_sample())
        self.assertEqual(a.get_instrument(), b.get_instrument())


if __name__ == "__main__":
    unittest.main()
