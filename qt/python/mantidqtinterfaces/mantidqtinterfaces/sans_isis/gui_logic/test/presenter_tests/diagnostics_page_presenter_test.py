# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest import mock
from sans.common.enums import SANSInstrument, DetectorType, IntegralEnum, SANSFacility
from mantidqtinterfaces.sans_isis.gui_logic.presenter.diagnostic_presenter import DiagnosticsPagePresenter
from mantidqtinterfaces.sans_isis.gui_logic.test.mock_objects import create_mock_diagnostics_tab
from mantidqtinterfaces.sans_isis.gui_logic.test.mock_objects import create_run_tab_presenter_mock


class DiagnosticsPagePresenterTest(unittest.TestCase):
    def setUp(self):
        self.parent_presenter = create_run_tab_presenter_mock(use_fake_state=False)
        self.view = create_mock_diagnostics_tab()
        self.presenter = DiagnosticsPagePresenter(self.parent_presenter, SANSFacility.ISIS)
        self.presenter.set_view(self.view, SANSInstrument.LARMOR)

        # Inject mocks to things tested elsewhere
        self.presenter._worker = mock.create_autospec(self.presenter._worker)
        self.presenter._model = mock.create_autospec(self.presenter._model)

        self.mock_state = self.presenter._model.create_state.return_value

    def test_that_on_horizontal_clicked_calls_worker_with_correct_parameters(self):
        self.presenter.on_horizontal_clicked()

        self.presenter._worker.run_integral.assert_called_once_with(
            self.view.horizontal_range, self.view.horizontal_mask, IntegralEnum.Horizontal, DetectorType.LAB, self.mock_state
        )

    def test_that_on_vertical_clicked_calls_worker_with_correct_parameters(self):
        self.presenter.on_vertical_clicked()

        self.presenter._worker.run_integral.assert_called_once_with(
            self.view.vertical_range, self.view.vertical_mask, IntegralEnum.Vertical, DetectorType.LAB, self.mock_state
        )

    def test_that_on_time_clicked_calls_worker_with_correct_parameters(self):
        self.presenter.on_time_clicked()

        self.presenter._worker.run_integral.assert_called_once_with(
            self.view.time_range, self.view.time_mask, IntegralEnum.Time, DetectorType.LAB, self.mock_state
        )

    def test_that_on_user_file_load_sets_user_file_name_on_view(self):
        user_file_name = "user_file_name"
        self.presenter.on_user_file_load(user_file_name)

        self.assertEqual(self.presenter._view.user_file_name, user_file_name)

    def test_that_set_instrument_settings_calls_set_detectors_on_view(self):
        self.presenter.set_instrument_settings(SANSInstrument.SANS2D)

        self.presenter._view.set_detectors.assert_called_with(["rear", "front"])
        self.assertEqual(self.presenter._view.set_detectors.call_count, 2)


if __name__ == "__main__":
    unittest.main()
