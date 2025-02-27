# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum
from typing import Dict

from mantid.api import AlgorithmFactory

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


class CategoryProperties(Enum):
    HIDDEN_ALGORITHMS = "algorithms.categories.hidden"
    HIDDEN_INTERFACES = "interfaces.categories.hidden"


class CategoriesSettingsModel(ConfigSettingsChangesModel):
    def __init__(self):
        super().__init__()
        self._algorithm_factory = AlgorithmFactory.Instance()

    def set_hidden_algorithms(self, value: str) -> None:
        self.add_change(CategoryProperties.HIDDEN_ALGORITHMS.value, value)

    def set_hidden_interfaces(self, value: str) -> None:
        self.add_change(CategoryProperties.HIDDEN_INTERFACES.value, value)

    def get_hidden_interfaces(self) -> str:
        return self.get_saved_value(CategoryProperties.HIDDEN_INTERFACES.value)

    def get_algorithm_factory_category_map(self) -> Dict[str, bool]:
        return self._algorithm_factory.getCategoriesandState()
