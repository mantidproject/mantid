# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
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

    @patch(tab_path + ".presenter.CalibrationPresenter.start_calibration_worker")
    def test_worker_started_with_right_params(self, worker_method):
        # Given
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)

        # When
        self.view.get_vanadium_filename.return_value = "307521"
        self.view.get_calib_filename.return_value = "305738"
        self.view.get_plot_output.return_value = True

        # Then
        self.presenter.on_calibrate_clicked()
        worker_method.assert_called_with("307521", "305738", True)

    def test_controls_disabled_while_running(self):
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)

        self.presenter.calibration_started()

        self.view.set_calibrate_button_enabled.assert_called_with(False)
        self.view.set_check_plot_output_enabled.assert_called_with(False)

    def test_controls_enabled_when_done_running(self):
        self.presenter = presenter.CalibrationPresenter(self.model, self.view)

        self.presenter.calibration_started()

        self.view.set_calibrate_button_enabled.assert_called_with(False)
        self.view.set_check_plot_output_enabled.assert_called_with(False)

        self.presenter.calibration_finished()

        self.view.set_calibrate_button_enabled.assert_called_with(True)
        self.view.set_check_plot_output_enabled.assert_called_with(True)
