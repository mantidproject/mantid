# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest.mock import MagicMock, patch, call

from workbench.widgets.settings.fitting.fitting_settings_model import FittingSettingsModel, FittingProperties
from workbench.widgets.settings.test_utilities.settings_model_test_base import BaseSettingsModelTest


class MockFunctionFactory:
    def __init__(self):
        self.getBackgroundFunctionNames = MagicMock()
        self.getPeakFunctionNames = MagicMock()


class FittingSettingsModelTest(BaseSettingsModelTest):
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
        self._assert_getter_with_different_values(
            mock_function_factory.getBackgroundFunctionNames,
            self.model.get_background_function_names,
            [["linear", "gaussian"], ["exp", "linear"]],
            call(),
        )

    def test_get_peak_function_names(self):
        mock_function_factory = self._add_mock_function_factory()
        self._assert_getter_with_different_values(
            mock_function_factory.getPeakFunctionNames,
            self.model.get_peak_function_names,
            [["linear", "gaussian"], ["exp", "linear"]],
            call(),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_auto_background(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_auto_background, ["linear", "log"], call(FittingProperties.AUTO_BACKGROUND.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_default_peak(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_default_peak, ["guassian", "expDecay"], call(FittingProperties.DEFAULT_PEAK.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_fwhm(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_fwhm, ["12", "0.7"], call(FittingProperties.FWHM.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_tolerance(self, get_saved_value_mock: MagicMock):
        self._assert_getter_with_different_values(
            get_saved_value_mock, self.model.get_tolerance, ["1", "5.7"], call(FittingProperties.TOLERANCE.value)
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_auto_background(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_auto_background, ["Ploynomial", "Linear"], FittingProperties.AUTO_BACKGROUND.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_default_peak(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_default_peak, ["Gaussian", "ExpDecay"], FittingProperties.DEFAULT_PEAK.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_fwhm(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(add_change_mock, self.model.set_fwhm, ["5", "11"], FittingProperties.FWHM.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_tolerance(self, add_change_mock: MagicMock):
        self._assert_setter_with_different_values(
            add_change_mock, self.model.set_tolerance, ["0.5", "1"], FittingProperties.TOLERANCE.value
        )
