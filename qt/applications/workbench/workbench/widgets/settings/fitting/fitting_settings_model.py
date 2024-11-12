# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum

from mantid.api import FunctionFactory

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


class FittingProperties(Enum):
    AUTO_BACKGROUND = "curvefitting.autoBackground"
    DEFAULT_PEAK = "curvefitting.defaultPeak"
    FWHM = "curvefitting.findPeaksFWHM"
    TOLERANCE = "curvefitting.findPeaksTolerance"


class FittingSettingsModel(ConfigSettingsChangesModel):
    def __init__(self):
        super().__init__()
        self.function_factory = FunctionFactory.Instance()

    def get_background_function_names(self):
        return self.function_factory.getBackgroundFunctionNames()

    def get_peak_function_names(self):
        return self.function_factory.getPeakFunctionNames()

    def get_auto_background(self) -> str:
        return self.get_saved_value(FittingProperties.AUTO_BACKGROUND.value)

    def get_default_peak(self) -> str:
        return self.get_saved_value(FittingProperties.DEFAULT_PEAK.value)

    def get_fwhm(self) -> str:
        return self.get_saved_value(FittingProperties.FWHM.value)

    def get_tolerance(self) -> str:
        return self.get_saved_value(FittingProperties.TOLERANCE.value)

    def set_auto_background(self, value: str) -> None:
        self.add_change(FittingProperties.AUTO_BACKGROUND.value, value)

    def set_default_peak(self, value: str) -> None:
        self.add_change(FittingProperties.DEFAULT_PEAK.value, value)

    def set_fwhm(self, value: str) -> None:
        self.add_change(FittingProperties.FWHM.value, value)

    def set_tolerance(self, value: str) -> None:
        self.add_change(FittingProperties.TOLERANCE.value, value)
