# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import unittest
from unittest.mock import patch, MagicMock

from workbench.widgets.settings.categories.categories_settings_model import CategoriesSettingsModel, CategoryProperties


class MockAlgorithmFactory:
    def __init__(self):
        self.getCategoriesandState = MagicMock()


class CategoriesSettingsModelTest(unittest.TestCase):
    mock_get_categories_and_state = MagicMock()

    def setUp(self) -> None:
        self.model = CategoriesSettingsModel()

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.add_change")
    def test_set_hidden_algorithms(self, add_change_mock: MagicMock):
        self.model.set_hidden_algorithms("Rebin;Load")
        add_change_mock.assert_called_once_with(CategoryProperties.HIDDEN_ALGORITHMS.value, "Rebin;Load")

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.add_change")
    def test_set_hidden_interfaces(self, add_change_mock: MagicMock):
        self.model.set_hidden_interfaces("Indirect; Muon; Reflectometry")
        add_change_mock.assert_called_once_with(CategoryProperties.HIDDEN_INTERFACES.value, "Indirect; Muon; Reflectometry")

    @patch("workbench.widgets.settings.categories.categories_settings_model.CategoriesSettingsModel.get_saved_value")
    def test_get_hidden_interfaces(self, get_saved_value_mock: MagicMock):
        self.model.get_hidden_interfaces()
        get_saved_value_mock.assert_called_once_with(CategoryProperties.HIDDEN_INTERFACES.value)

    def test_get_algorithm_factory_category_map(self):
        mock_alg_factory = MockAlgorithmFactory()
        self.model.algorithm_factory = mock_alg_factory
        self.model.get_algorithm_factory_category_map()
        mock_alg_factory.getCategoriesandState.assert_called_once()
