# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest.mock import patch, MagicMock, call

from workbench.widgets.settings.categories.categories_settings_model import CategoriesSettingsModel, CategoryProperties
from workbench.widgets.settings.test_utilities.settings_model_test_base import BaseSettingsModelTest


class MockAlgorithmFactory:
    def __init__(self):
        self.getCategoriesandState = MagicMock()


class CategoriesSettingsModelTest(BaseSettingsModelTest):
    mock_get_categories_and_state = MagicMock()

    def setUp(self) -> None:
        self.model = CategoriesSettingsModel()

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.add_change")
    def test_set_hidden_algorithms(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_hidden_algorithms, ["Rebin;Load", "Mean"], CategoryProperties.HIDDEN_ALGORITHMS.value
        )

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.add_change")
    def test_set_hidden_interfaces(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_hidden_interfaces,
            ["Indirect; Muon; Reflectometry", "Engineering Diffraction"],
            CategoryProperties.HIDDEN_INTERFACES.value,
        )

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.get_saved_value")
    def test_get_hidden_interfaces(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_hidden_interfaces,
            ["Indirect; Muon; Reflectometry", "Engineering Diffraction"],
            call(CategoryProperties.HIDDEN_INTERFACES.value),
        )

    def test_get_algorithm_factory_category_map(self):
        mock_alg_factory = MockAlgorithmFactory()
        self.model._algorithm_factory = mock_alg_factory
        self._test_getter_with_different_values(
            mock_alg_factory.getCategoriesandState,
            self.model.get_algorithm_factory_category_map,
            [{"Basic": "Load"}, {"Arithmetic": "Mean"}],
            call(),
        )
