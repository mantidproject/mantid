# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import Mock, MagicMock, patch
from mantidqt.utils.qt.testing import start_qapplication
from workbench.widgets.settings.fitting.presenter import FittingSettings
from workbench.widgets.settings.test_utilities.settings_test_utilities import (
    assert_presenter_has_added_mousewheel_filter_to_all_como_and_spin_boxes,
)


class MockSettingsView(object):
    def __init__(self):
        self.auto_bkg = Mock()
        self.background_args = Mock()
        self.default_peak = Mock()
        self.findpeaks_fwhm = Mock()
        self.findpeaks_tol = Mock()


class MockFittingSettingsModel:
    def __init__(self):
        self.get_background_function_names = MagicMock()
        self.get_peak_function_names = MagicMock()
        self.get_auto_background = MagicMock()
        self.get_default_peak = MagicMock()
        self.get_fwhm = MagicMock()
        self.get_tolerance = MagicMock()
        self.set_auto_background = MagicMock()
        self.set_default_peak = MagicMock()
        self.set_fwhm = MagicMock()
        self.set_tolerance = MagicMock()


MOUSEWHEEL_EVENT_FILTER_PATH = "workbench.widgets.settings.fitting.presenter.filter_out_mousewheel_events_from_combo_or_spin_box"


@start_qapplication
@patch(MOUSEWHEEL_EVENT_FILTER_PATH)
class FittingSettingsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.mock_view = MockSettingsView()
        cls.mock_model = MockFittingSettingsModel()

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @staticmethod
    def test_filters_added_to_combo_and_spin_boxes(self, mock_mousewheel_filter):
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)
        view = presenter.get_view()
        assert_presenter_has_added_mousewheel_filter_to_all_como_and_spin_boxes(view, mock_mousewheel_filter)

    def test_setup_signals(self, _):
        self.mock_view.auto_bkg.currentTextChanged.connect = MagicMock()
        self.mock_view.background_args.editingFinished.connect = MagicMock()
        self.mock_view.default_peak.currentTextChanged.connect = MagicMock()
        self.mock_view.findpeaks_fwhm.valueChanged.connect = MagicMock()
        self.mock_view.findpeaks_tol.valueChanged.connect = MagicMock()

        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        self.mock_view.auto_bkg.currentTextChanged.connect.assert_called_once_with(presenter.action_auto_background_changed)
        self.mock_view.background_args.editingFinished.connect.assert_called_once_with(presenter.action_background_args_changed)
        self.mock_view.default_peak.currentTextChanged.connect.assert_called_once_with(presenter.action_default_peak_changed)
        self.mock_view.findpeaks_fwhm.valueChanged.connect.assert_called_once_with(presenter.action_find_peaks_fwhm_changed)
        self.mock_view.findpeaks_tol.valueChanged.connect.assert_called_once_with(presenter.action_find_peaks_tolerance_changed)

    def test_action_auto_background_changed(self, _):
        self.mock_view.background_args.text = Mock(return_value="")
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        self.mock_model.set_auto_background.reset_mock()

        presenter.action_auto_background_changed("None")
        self.mock_model.set_auto_background.assert_called_once_with("")

        self.mock_model.set_auto_background.reset_mock()

        presenter.action_auto_background_changed("Polynomial")
        self.mock_model.set_auto_background.assert_called_once_with("Polynomial ")

    def test_action_background_args_changed(self, _):
        self.mock_view.auto_bkg.currentText = Mock(return_value="Polynomial")
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        self.mock_model.set_auto_background.reset_mock()

        self.mock_view.background_args.text = Mock(return_value="n=3")
        presenter.action_background_args_changed()
        self.mock_model.set_auto_background.assert_called_once_with("Polynomial n=3")

        self.mock_model.set_auto_background.reset_mock()

        self.mock_view.background_args.text = Mock(return_value="n=5")
        presenter.action_background_args_changed()
        self.mock_model.set_auto_background.assert_called_once_with("Polynomial n=5")

    def test_action_background_args_changed_with_auto_background_none(self, _):
        self.mock_view.auto_bkg.currentText = Mock(return_value="None")
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        self.mock_model.set_auto_background.reset_mock()

        self.mock_view.background_args.text = Mock(return_value="n=3")
        presenter.action_background_args_changed()
        self.mock_model.set_auto_background.assert_called_once_with("")

    def test_action_default_peak_changed(self, _):
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        presenter.action_default_peak_changed("None")
        self.mock_model.set_default_peak.assert_called_once_with("None")

        self.mock_model.set_default_peak.reset_mock()

        presenter.action_default_peak_changed("Gaussian")
        self.mock_model.set_default_peak.assert_called_once_with("Gaussian")

    def test_action_find_peaks_fwhm_changed(self, _):
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        presenter.action_find_peaks_fwhm_changed(5)
        self.mock_model.set_fwhm.assert_called_once_with("5")

        self.mock_model.set_fwhm.reset_mock()

        presenter.action_find_peaks_fwhm_changed(9)
        self.mock_model.set_fwhm.assert_called_once_with("9")

    def test_action_find_peaks_tolerance_changed(self, _):
        presenter = FittingSettings(None, view=self.mock_view, model=self.mock_model)

        presenter.action_find_peaks_tolerance_changed(3)
        self.mock_model.set_tolerance.assert_called_once_with("3")

        self.mock_model.set_tolerance.reset_mock()

        presenter.action_find_peaks_tolerance_changed(8)
        self.mock_model.set_tolerance.assert_called_once_with("8")
