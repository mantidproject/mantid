# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import TestCase
from unittest.mock import patch, call

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel
from workbench.widgets.settings.test_utilities.mock_config_service import MockConfigService, BASE_CLASS_CONFIG_SERVICE_PATCH_PATH


@patch(BASE_CLASS_CONFIG_SERVICE_PATCH_PATH, new_callable=MockConfigService)
class ConfigSettingsChangesModelTest(TestCase):
    def setUp(self) -> None:
        self.model = ConfigSettingsChangesModel()

    def test_add_change_adds_to_changes(self, mock_config_service: MockConfigService):
        self.model.add_change("property.1", "blue")

        mock_config_service.getString.assert_called_once_with("property.1")
        self.assertEqual(self.model.get_changes(), {"property.1": "blue"})

        self.model.add_change("property.2", "apple")

        mock_config_service.getString.assert_called_with("property.2")
        self.assertEqual(self.model.get_changes(), {"property.1": "blue", "property.2": "apple"})

    def test_add_change_doesnt_add_change_identical_to_saved_value(self, mock_config_service: MockConfigService):
        mock_config_service.getString.return_value = "green"
        self.model.add_change("property.1", "green")

        self.assertEqual(self.model.get_changes(), {})

    def test_add_change_removes_change_if_already_saved(self, mock_config_service: MockConfigService):
        mock_config_service.getString.return_value = "green"
        self.model.add_change("property.1", "blue")

        self.assertEqual(self.model.get_changes(), {"property.1": "blue"})

        self.model.add_change("property.1", "green")

        self.assertEqual(self.model.get_changes(), {})

    def test_has_unsaved_changes(self, _):
        self.assertFalse(self.model.has_unsaved_changes())

        self.model.add_change("property.1", "blue")

        self.assertTrue(self.model.has_unsaved_changes())

    def test_get_changes(self, _):
        self.assertEqual(self.model.get_changes(), {})

        self.model._changes = {"a test": "change"}

        self.assertEqual(self.model.get_changes(), {"a test": "change"})

        self.model._changes = {"something": "else"}

        self.assertEqual(self.model.get_changes(), {"something": "else"})

    def test_properties_to_be_changed(self, _):
        self.assertEqual(self.model.properties_to_be_changed(), [])

        self.model.add_change("property.1", "blue")
        self.model.add_change("property.2", "apple")

        self.assertEqual(self.model.properties_to_be_changed(), ["property.1", "property.2"])

    def test_get_saved_value(self, mock_config_service: MockConfigService):
        def side_effect_return(property_string: str):
            mock_saved_values = {"property.1": "blue", "property.2": "apple"}
            return mock_saved_values[property_string]

        mock_config_service.getString.side_effect = side_effect_return

        self.assertEqual(self.model.get_saved_value("property.1"), "blue")
        self.assertEqual(self.model.get_saved_value("property.2"), "apple")
        mock_config_service.getString.assert_has_calls([call("property.1"), call("property.2")])

    def test_apply_changes(self, mock_config_service: MockConfigService):
        self.model.add_change("property.1", "blue")
        self.model.add_change("property.2", "apple")

        self.model.apply_changes()

        mock_config_service.setString.assert_has_calls([call("property.1", "blue"), call("property.2", "apple")])
        self.assertEqual(self.model.get_changes(), {})
