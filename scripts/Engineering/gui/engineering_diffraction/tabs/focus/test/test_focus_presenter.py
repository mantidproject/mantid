# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from unittest.mock import patch
from Engineering.gui.engineering_diffraction.tabs.focus import model, view, presenter
from Engineering.gui.engineering_diffraction.tabs.common.calibration_info import CalibrationInfo

tab_path = "Engineering.gui.engineering_diffraction.tabs.focus"


class FocusPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = mock.create_autospec(view.FocusView)
        self.model = mock.create_autospec(model.FocusModel)
        self.presenter = presenter.FocusPresenter(self.model, self.view)
        self.presenter.current_calibration.set_roi_info(None, None, None)

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params(self, worker, setting):
        self.presenter.current_calibration = CalibrationInfo(sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.get_focus_filenames.return_value = "305738"
        self.view.get_vanadium_filename.return_value = "307521"
        # testing south bank roi info is correctly propagated
        self.presenter.current_calibration.set_roi_info('2', None, None)
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False

        self.presenter.on_focus_clicked()
        grp_dict_south = {'bank_2': 'SouthBank_grouping'}
        worker.assert_called_with("305738", "307521", True, None, grp_dict_south)
        self.assertEqual(1, setting.call_count)

    @patch(tab_path + ".presenter.set_setting")
    @patch(tab_path + ".presenter.FocusPresenter.start_focus_worker")
    def test_worker_started_with_correct_params_custom_crop(self, worker, setting):
        self.presenter.current_calibration = CalibrationInfo(sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.get_focus_filenames.return_value = "305738"
        self.view.get_vanadium_filename.return_value = "307521"
        self.presenter.current_calibration.set_roi_info(None, None, "2-45")
        self.view.get_plot_output.return_value = True
        self.view.is_searching.return_value = False
        rp_dict_cropped = {'Cropped': 'Custom_spectra_grouping'}

        self.presenter.on_focus_clicked()
        worker.assert_called_with("305738", "307521", True, None, rp_dict_cropped)
        self.assertEqual(1, setting.call_count)

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
        self.presenter.current_calibration = CalibrationInfo(sample_path=None,
                                                             instrument=None)
        self.view.is_searching.return_value = False

        self.presenter._validate()
        create_error.assert_called_with(
            self.presenter.view,
            "Create or Load a calibration via the Calibration tab before focusing.")

    @patch(tab_path + ".presenter.create_error_message")
    def test_validate_while_searching(self, create_error):
        self.presenter.current_calibration = CalibrationInfo(sample_path="Fake/Path",
                                                             instrument="ENGINX")
        self.view.is_searching.return_value = True

        self.assertEqual(False, self.presenter._validate())
        self.assertEqual(1, create_error.call_count)


if __name__ == '__main__':
    unittest.main()
