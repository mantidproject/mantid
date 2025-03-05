# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch
from mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus import model, view, presenter
from Engineering.common.calibration_info import CalibrationInfo

tab_path = "mantidqtinterfaces.Engineering.gui.engineering_diffraction.tabs.focus"


class FocusPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FocusView, instance=True)
        self.model = mock.create_autospec(model.FocusModel, instance=True)
        self.presenter = presenter.FocusPresenter(self.model, self.view)
        self.presenter.current_calibration = mock.create_autospec(CalibrationInfo, instance=True)

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    @patch(tab_path + ".presenter.FocusPresenter._validate")
    def test_worker_started_with_correct_params(self, mock_validate, mock_worker, mock_setting):
        self.view.get_focus_filenames.return_value = "305738"
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_plot_output.return_value = True
        mock_validate.return_value = True

        self.presenter.on_focus_clicked()

        mock_worker.assert_called_with("305738", "307521", True, None, self.presenter.current_calibration)
        self.assertEqual(1, mock_setting.call_count)

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    @patch(tab_path + ".presenter.FocusPresenter._validate")
    def test_worker_not_started_when_validate_fails(self, mock_validate, mock_worker, mock_setting):
        mock_validate.return_value = False

        self.presenter.on_focus_clicked()

        mock_worker.assert_not_called()
        mock_setting.assert_not_called()

    def test_controls_disabled_disables_both(self):
        self.presenter.set_focus_controls_enabled(False)

        self.view.set_focus_button_enabled.assert_called_with(False)
        self.view.set_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_enables_both(self):
        self.presenter.set_focus_controls_enabled(True)

        self.view.set_focus_button_enabled.assert_called_with(True)
        self.view.set_plot_output_enabled.assert_called_with(True)

    @patch(tab_path + ".presenter.FocusPresenter.emit_enable_button_signal")
    def test_on_worker_error_enables_controls(self, emit):
        fail_info = 2024278

        self.presenter._on_worker_error(fail_info)

        self.assertEqual(1, emit.call_count)

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_focus_path(self, error_message):
        self.view.get_focus_valid.return_value = False

        self.assertFalse(self.presenter._validate())
        self.assertEqual(error_message.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_van_path(self, error_message):
        self.view.get_vanadium_valid.return_value = False

        self.assertFalse(self.presenter._validate())
        self.assertEqual(error_message.call_count, 1)

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_with_invalid_calibration(self, create_error):
        self.presenter.current_calibration.is_valid.return_value = False
        self.view.is_searching.return_value = False

        self.assertFalse(self.presenter._validate())
        create_error.assert_called_with(self.presenter.view, "Create or Load a calibration via the Calibration tab before focusing.")

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_while_searching(self, create_error):
        self.view.is_searching.return_value = True

        self.assertFalse(self.presenter._validate())
        create_error.assert_called_with(self.presenter.view, "Mantid is searching for data files. Please wait.")


if __name__ == "__main__":
    unittest.main()
