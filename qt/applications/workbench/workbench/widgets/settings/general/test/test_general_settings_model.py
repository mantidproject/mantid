# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest.mock import MagicMock, patch, call

from workbench.widgets.settings.general.general_settings_model import GeneralSettingsModel, GeneralProperties, GeneralUserConfigProperties
from workbench.widgets.settings.test_utilities.mock_config_service import MockConfigService, BASE_CLASS_CONFIG_SERVICE_PATCH_PATH
from workbench.widgets.settings.test_utilities.settings_model_test_base import BaseSettingsModelTest


class MockUserConfig:
    def __init__(self):
        self.get = MagicMock()
        self.has = MagicMock(return_value=True)
        self.set = MagicMock()


class GeneralSettingsModelTest(BaseSettingsModelTest):
    USER_CONFIG_PATCH_PATH = "workbench.widgets.settings.general.general_settings_model.CONF"
    GET_SAVED_VALUE_PATCH_PATH = "workbench.widgets.settings.general.general_settings_model.GeneralSettingsModel.get_saved_value"
    ADD_CHANGE_PATCH_PATH = "workbench.widgets.settings.general.general_settings_model.GeneralSettingsModel.add_change"

    def setUp(self) -> None:
        self.model = GeneralSettingsModel()

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    @patch(BASE_CLASS_CONFIG_SERVICE_PATCH_PATH, new_callable=MockConfigService)
    def test_add_change_adds_to_correct_changes_dict(self, _, __):
        expected_changes = {}
        for property_value in [prop.value for prop in GeneralProperties]:
            self.model.add_change(property_value, "example_value")
            expected_changes[property_value] = "example_value"

        expected_user_config_changes = {}
        for user_property_value in [user_prop.value for user_prop in GeneralUserConfigProperties]:
            self.model.add_change(user_property_value, "example_value")
            expected_user_config_changes[user_property_value] = "example_value"

        self.assertEqual(self.model.changes, expected_changes)
        self.assertEqual(self.model.user_config_changes, expected_user_config_changes)

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_add_change_doesnt_add_change_identical_to_user_config(self, mock_user_config: MockUserConfig):
        mock_user_config.get.return_value = "calibri"
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "calibri")

        self.assertEqual(self.model.user_config_changes, {})

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_add_change_removes_change_if_already_saved(self, mock_user_config: MockUserConfig):
        mock_user_config.get.return_value = "calibri"
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")

        self.assertEqual(self.model.user_config_changes, {GeneralUserConfigProperties.FONT.value: "times new roman"})

        self.model.add_change(GeneralUserConfigProperties.FONT.value, "calibri")

        self.assertEqual(self.model.user_config_changes, {})

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_add_change_calls_get_with_correct_type(self, mock_user_config: MockUserConfig):
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")
        self.model.add_change(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, True)
        self.model.add_change(GeneralUserConfigProperties.USER_LAYOUT.value, {"example": "dictionary"})

        mock_user_config.get.assert_any_call(GeneralUserConfigProperties.FONT.value, type=str)
        mock_user_config.get.assert_any_call(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, type=bool)
        mock_user_config.get.assert_any_call(GeneralUserConfigProperties.USER_LAYOUT.value, type=dict)

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_add_change_handles_user_config_options_not_yet_saved(self, mock_user_config: MockUserConfig):
        mock_user_config.has.return_value = False
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")

        mock_user_config.get.assert_not_called()
        self.assertEqual(self.model.user_config_changes, {GeneralUserConfigProperties.FONT.value: "times new roman"})

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    @patch(BASE_CLASS_CONFIG_SERVICE_PATCH_PATH, new_callable=MockConfigService)
    def test_has_unsaved_changes(self, mock_config_service: MockConfigService, mock_user_config: MockUserConfig):
        mock_user_config.get.return_value = "calibri"
        mock_config_service.getString.return_value = "Off"
        self.assertFalse(self.model.has_unsaved_changes())

        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")

        self.assertTrue(self.model.has_unsaved_changes())

        self.model.add_change(GeneralProperties.CRYSTALLOGRAPY_CONV.value, "On")

        self.assertTrue(self.model.has_unsaved_changes())

        self.model.add_change(GeneralUserConfigProperties.FONT.value, "calibri")

        self.assertTrue(self.model.has_unsaved_changes())

        self.model.add_change(GeneralProperties.CRYSTALLOGRAPY_CONV.value, "Off")

        self.assertFalse(self.model.has_unsaved_changes())

    def test_get_changes_dict(self):
        self.assertEqual(self.model.get_changes_dict(), {})

        self.model.user_config_changes = {"user property": "user change"}

        self.assertEqual(self.model.get_changes_dict(), {"user property": "user change"})

        self.model.changes = {"config property": "config change"}

        self.assertEqual(self.model.get_changes_dict(), {"user property": "user change", "config property": "config change"})

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    @patch(BASE_CLASS_CONFIG_SERVICE_PATCH_PATH, new_callable=MockConfigService)
    def test_properties_to_be_changed(self, _, __):
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")
        self.model.add_change(GeneralProperties.CRYSTALLOGRAPY_CONV.value, "On")

        self.assertEqual(
            self.model.properties_to_be_changed(), [GeneralUserConfigProperties.FONT.value, GeneralProperties.CRYSTALLOGRAPY_CONV.value]
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_apply_changes(self, mock_user_config: MockUserConfig):
        self.model.add_change(GeneralUserConfigProperties.FONT.value, "times new roman")
        self.model.add_change(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, True)

        self.model.apply_changes()

        mock_user_config.set.assert_has_calls(
            [
                call(GeneralUserConfigProperties.FONT.value, "times new roman"),
                call(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, True),
            ]
        )
        self.assertEqual(self.model.user_config_changes, {})

    @patch("workbench.widgets.settings.general.general_settings_model.ConfigService", new_callable=MockConfigService)
    def test_apply_changes_sets_facility_with_config_service(self, mock_config_service: MockConfigService):
        mock_config_service.setFacility = MagicMock()
        self.model.add_change(GeneralProperties.FACILITY.value, "new_facility_123")

        self.model.apply_changes()

        mock_config_service.setFacility.assert_called_once_with("new_facility_123")
        self.assertEqual(self.model.changes, {})

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_crystallography_convention(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_crystallography_convention,
            ["On", "Off"],
            call(GeneralProperties.CRYSTALLOGRAPY_CONV.value),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_facility(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock, self.model.get_facility, ["ISIS", "ILL"], call(GeneralProperties.FACILITY.value)
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_font(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get, self.model.get_font, ["calibri", "papyrus"], call(GeneralUserConfigProperties.FONT.value, type=str)
        )

        mock_user_config.has.return_value = False
        self.assertIsNone(self.model.get_font())

    @patch("workbench.widgets.settings.general.general_settings_model.ConfigService", new_callable=MockConfigService)
    def test_get_facility_names(self, mock_config_service: MockConfigService):
        mock_config_service.getFacilityNames = MagicMock()
        self._test_getter_with_different_values(
            mock_config_service.getFacilityNames,
            self.model.get_facility_names,
            [["facility 1", "facility 2"], ["facility 3", "facility 4"]],
            call(),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_instrument(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock, self.model.get_instrument, ["EMU", "WISH"], call(GeneralProperties.INSTRUMENT.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_use_opengl(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock, self.model.get_use_opengl, ["On", "Off"], call(GeneralProperties.OPENGL.value)
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_show_invisible_workspaces(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_show_invisible_workspaces,
            ["On", "Off"],
            call(GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_project_recovery_number_of_checkpoints(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_project_recovery_number_of_checkpoints,
            ["57", "23"],
            call(GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_project_recovery_time_between_recoveries(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock,
            self.model.get_project_recovery_time_between_recoveries,
            ["7", "13"],
            call(GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_project_recovery_enabled(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock, self.model.get_project_recovery_enabled, ["On", "Off"], call(GeneralProperties.PR_RECOVERY_ENABLED.value)
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_prompt_on_deleting_workspace(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_prompt_on_deleting_workspace,
            [True, False],
            call(GeneralUserConfigProperties.PROMPT_ON_DELETING_WORKSPACE.value, type=bool),
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_prompt_on_save_editor_modified(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_prompt_on_save_editor_modified,
            [True, False],
            call(GeneralUserConfigProperties.PROMPT_SAVE_EDITOR_MODIFIED.value, type=bool),
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_prompt_save_on_close(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_prompt_save_on_close,
            [True, False],
            call(GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value, type=bool),
        )

    @patch(GET_SAVED_VALUE_PATCH_PATH)
    def test_get_use_notifications(self, get_saved_value_mock: MagicMock):
        self._test_getter_with_different_values(
            get_saved_value_mock, self.model.get_use_notifications, ["On", "Off"], call(GeneralProperties.USE_NOTIFICATIONS.value)
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_user_layout(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_user_layout,
            [{"test": 123}, {"another": "one"}],
            call(GeneralUserConfigProperties.USER_LAYOUT.value, type=dict),
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_user_layout_can_return_potential_change(self, mock_user_config: MockUserConfig):
        self.model.add_change(GeneralUserConfigProperties.USER_LAYOUT.value, {"my cool": "layout"})
        self.assertEqual(self.model.get_user_layout(get_potential_update=True), {"my cool": "layout"})

        self.model.user_config_changes.clear()
        mock_user_config.get.reset_mock()
        mock_user_config.get.return_value = {"saved": "value"}
        self.assertEqual(self.model.get_user_layout(get_potential_update=True), {"saved": "value"})
        mock_user_config.get.assert_called_once_with(GeneralUserConfigProperties.USER_LAYOUT.value, type=dict)

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_window_behaviour(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_window_behaviour,
            ["Floating", "On top"],
            call(GeneralUserConfigProperties.WINDOW_BEHAVIOUR.value, type=str),
        )

    @patch(USER_CONFIG_PATCH_PATH, new_callable=MockUserConfig)
    def test_get_completion_enabled(self, mock_user_config: MockUserConfig):
        self._test_getter_with_different_values(
            mock_user_config.get,
            self.model.get_completion_enabled,
            [True, False],
            call(GeneralUserConfigProperties.COMPLETION_ENABLED.value, type=bool),
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_crystallography_convention(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_crystallography_convention, ["On", "Off"], GeneralProperties.CRYSTALLOGRAPY_CONV.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_facility(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(add_change_mock, self.model.set_facility, ["ISIS", "ILL"], GeneralProperties.FACILITY.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_font(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_font, ["comic sans", "helvetica"], GeneralUserConfigProperties.FONT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_window_behaviour(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_window_behaviour, ["On top", "floating"], GeneralUserConfigProperties.WINDOW_BEHAVIOUR.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_completion_enabled(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_completion_enabled, [False, True], GeneralUserConfigProperties.COMPLETION_ENABLED.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_prompt_save_on_closed(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_prompt_save_on_close, [True, False], GeneralUserConfigProperties.PROMPT_SAVE_ON_CLOSE.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_prompt_on_save_editor_modified(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_prompt_on_save_editor_modified,
            [True, False],
            GeneralUserConfigProperties.PROMPT_SAVE_EDITOR_MODIFIED.value,
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_prompt_on_deleting_workspace(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_prompt_on_deleting_workspace,
            [False, True],
            GeneralUserConfigProperties.PROMPT_ON_DELETING_WORKSPACE.value,
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_use_notifications(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_use_notifications, ["Off", "On"], GeneralProperties.USE_NOTIFICATIONS.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_project_recovery_enabled(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_project_recovery_enabled, ["Off", "On"], GeneralProperties.PR_RECOVERY_ENABLED.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_project_recovery_time_between_recoveries(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_project_recovery_time_between_recoveries,
            ["55", "138"],
            GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value,
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_project_recovery_number_of_checkpoints(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_project_recovery_number_of_checkpoints,
            ["5", "10"],
            GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value,
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_instrument(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_instrument, ["ALF", "SXD"], GeneralProperties.INSTRUMENT.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_show_invisible_workspaces(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock, self.model.set_show_invisible_workspaces, ["Off", "On"], GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value
        )

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_use_opengl(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(add_change_mock, self.model.set_use_opengl, ["On", "Off"], GeneralProperties.OPENGL.value)

    @patch(ADD_CHANGE_PATCH_PATH)
    def test_set_user_layour(self, add_change_mock: MagicMock):
        self._test_setter_with_different_values(
            add_change_mock,
            self.model.set_user_layout,
            [{"a test": "dictionary"}, {"another": "one"}],
            GeneralUserConfigProperties.USER_LAYOUT.value,
        )
