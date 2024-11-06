# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest import TestCase
from unittest.mock import MagicMock, patch

from workbench.widgets.settings.fitting.fitting_settings_model import FittingSettingsModel, FittingProperties


class MockFunctionFactory:
    def __init__(self):
        self.getBackgroundFunctionNames = MagicMock()
        self.getPeakFunctionNames = MagicMock()


class FittingSettingsModelTest(TestCase):
    GET_SAVED_VALUE_PATCH_PATH = "workbench.widgets.settings.fitting.fitting_settings_model.FittingSettingsModel.get_saved_value"
    ADD_CHANGE_PATCH_PATH = "workbench.widgets.settings.fitting.fitting_settings_model.FittingSettingsModel.add_change"

    def setUp(self) -> None:
        self.model = FittingSettingsModel()

    def _add_mock_function_factory(self) -> MockFunctionFactory:
        mock_function_factory = MockFunctionFactory()
        self.model.function_factory = mock_function_factory
        return mock_function_factory

    def test_get_background_function_names(self):
        mock_function_factory = self._add_mock_function_factory()
        self.model.get_background_function_names()
        mock_function_factory.getBackgroundFunctionNames.assert_called_once()

    def test_get_peak_function_names(self):
        mock_function_factory = self._add_mock_function_factory()
        self.model.get_peak_function_names()
        mock_function_factory.getBackgroundFunctionNames.getPeakFunctionNames()

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_auto_background(self, get_saved_value_mock: MagicMock):
        self.model.get_auto_background()
        get_saved_value_mock.assert_called_once_with(FittingProperties.AUTO_BACKGROUND.value)

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_default_peak(self, get_saved_value_mock: MagicMock):
        self.model.get_default_peak()
        get_saved_value_mock.assert_called_once_with(FittingProperties.DEFAULT_PEAK.value)

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_fwhm(self, get_saved_value_mock: MagicMock):
        self.model.get_fwhm()
        get_saved_value_mock.assert_called_once_with(FittingProperties.FWHM.value)

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_tolerance(self, get_saved_value_mock: MagicMock):
        self.model.get_tolerance()
        get_saved_value_mock.assert_called_once_with(FittingProperties.TOLERANCE.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_auto_background(self, add_change_mock: MagicMock):
        self.model.set_auto_background("Polynomial")
        add_change_mock.assert_called_once_with(FittingProperties.AUTO_BACKGROUND.value, "Polynomial")

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_default_peak(self, add_change_mock: MagicMock):
        self.model.set_default_peak("Gaussian")
        add_change_mock.assert_called_once_with(FittingProperties.DEFAULT_PEAK.value, "Gaussian")

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_fwhm(self, add_change_mock: MagicMock):
        self.model.set_fwhm("5")
        add_change_mock.assert_called_once_with(FittingProperties.FWHM.value, "5")

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_tolerance(self, add_change_mock: MagicMock):
        self.model.set_tolerance("0.5")
        add_change_mock.assert_called_once_with(FittingProperties.TOLERANCE.value, "0.5")
