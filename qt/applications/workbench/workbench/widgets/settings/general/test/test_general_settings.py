# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import unittest

from unittest.mock import call, patch, MagicMock, Mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.general.presenter import GeneralSettings
from workbench.widgets.settings.general.general_settings_model import GeneralSettingsModel
from workbench.config import SAVE_STATE_VERSION
from qtpy.QtCore import Qt


class MockInstrument(object):
    def __init__(self, idx):
        self.name = StrictMock(return_value="instr{}".format(idx))


class MockFacility(object):
    def __init__(self, name):
        self.name = StrictMock(return_value=name)
        self.all_instruments = [MockInstrument(0), MockInstrument(1)]
        self.instruments = StrictMock(return_value=self.all_instruments)


class MockGeneralSettingsModel:
    def __init__(self):
        self.register_apply_callback = MagicMock()
        self.get_crystallography_convention = MagicMock()
        self.get_facility = MagicMock()
        self.get_font = MagicMock()
        self.get_facility_names = MagicMock()
        self.get_instrument = MagicMock()
        self.get_use_opengl = MagicMock()
        self.get_show_invisible_workspaces = MagicMock()
        self.get_project_recovery_number_of_checkpoints = MagicMock()
        self.get_project_recovery_time_between_recoveries = MagicMock()
        self.get_project_recovery_enabled = MagicMock()
        self.get_prompt_on_deleting_workspace = MagicMock()
        self.get_prompt_on_save_editor_modified = MagicMock()
        self.get_prompt_save_on_close = MagicMock()
        self.get_use_notifications = MagicMock()
        self.get_user_layout = MagicMock()
        self.get_window_behaviour = MagicMock()
        self.get_completion_enabled = MagicMock()
        self.set_crystallography_convention = MagicMock()
        self.set_facility = MagicMock()
        self.set_font = MagicMock()
        self.set_window_behaviour = MagicMock()
        self.set_completion_enabled = MagicMock()
        self.set_prompt_save_on_close = MagicMock()
        self.set_prompt_on_save_editor_modified = MagicMock()
        self.set_prompt_on_deleting_workspace = MagicMock()
        self.set_use_notifications = MagicMock()
        self.set_project_recovery_enabled = MagicMock()
        self.set_project_recovery_time_between_recoveries = MagicMock()
        self.set_project_recovery_number_of_checkpoints = MagicMock()
        self.set_instrument = MagicMock()
        self.set_show_invisible_workspaces = MagicMock()
        self.set_use_opengl = MagicMock()
        self.set_user_layout = MagicMock()


@start_qapplication
class GeneralSettingsTest(unittest.TestCase):
    MOUSEWHEEL_EVENT_FILTER_PATH = "workbench.widgets.settings.general.presenter.filter_out_mousewheel_events_from_combo_or_spin_box"

    def setUp(self) -> None:
        self.mock_model = MockGeneralSettingsModel()

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_filters_added_to_combo_and_spin_boxes(self, mock_mousewheel_filter):
        presenter = GeneralSettings(None, model=GeneralSettingsModel())

        calls = [
            call(presenter.get_view().facility),
            call(presenter.get_view().instrument),
            call(presenter.get_view().window_behaviour),
            call(presenter.get_view().time_between_recovery),
            call(presenter.get_view().total_number_checkpoints),
        ]

        mock_mousewheel_filter.assert_has_calls(calls, any_order=True)

    def test_setup_facilities_with_valid_combination(self):
        mock_facility = MockFacility("facility1")
        self.mock_model.get_facility.return_value = mock_facility.name()
        mock_instrument = mock_facility.all_instruments[0]
        self.mock_model.get_instrument.return_value = mock_instrument.name()
        self.mock_model.get_facility_names.return_value = ["facility1", "facility2"]

        presenter = GeneralSettings(None, model=self.mock_model)
        self.assertEqual(1, self.mock_model.set_facility.call_count)
        self.assertEqual(1, self.mock_model.get_facility.call_count)
        self.assert_connected_once(presenter.get_view().facility, presenter.get_view().facility.currentTextChanged)

        self.assertEqual(1, self.mock_model.get_instrument.call_count)
        self.assert_connected_once(presenter.get_view().instrument, presenter.get_view().instrument.currentTextChanged)

    def test_setup_checkbox_signals(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        self.assert_connected_once(
            presenter.get_view().crystallography_convention, presenter.get_view().crystallography_convention.stateChanged
        )

        self.assert_connected_once(presenter.get_view().use_open_gl, presenter.get_view().use_open_gl.stateChanged)

        self.assert_connected_once(
            presenter.get_view().show_invisible_workspaces, presenter.get_view().show_invisible_workspaces.stateChanged
        )

        self.assert_connected_once(
            presenter.get_view().project_recovery_enabled, presenter.get_view().project_recovery_enabled.stateChanged
        )

        self.assert_connected_once(presenter.get_view().time_between_recovery, presenter.get_view().time_between_recovery.valueChanged)

        self.assert_connected_once(
            presenter.get_view().total_number_checkpoints, presenter.get_view().total_number_checkpoints.valueChanged
        )

        self.assert_connected_once(presenter.get_view().completion_enabled, presenter.get_view().completion_enabled.stateChanged)

    def test_font_dialog_signals(self):
        presenter = GeneralSettings(None, model=self.mock_model)
        with patch.object(presenter.get_view(), "create_font_dialog", MagicMock()) as font_dialog:
            presenter.action_main_font_button_clicked()
            self.assertEqual(1, font_dialog().fontSelected.connect.call_count)

    def test_setup_general_group_signals(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        self.assert_connected_once(presenter.get_view().main_font, presenter.get_view().main_font.clicked)
        self.assert_connected_once(presenter.get_view().window_behaviour, presenter.get_view().window_behaviour.currentTextChanged)

    def test_action_facility_changed(self):
        self.mock_model.get_facility_names.return_value = ["facility1", "facility2"]
        self.mock_model.get_facility.return_value = "facility1"
        presenter = GeneralSettings(None, model=self.mock_model)
        self.mock_model.set_facility.reset_mock()
        self.mock_model.set_instrument.reset_mock()

        new_facility = "TEST_LIVE"
        default_test_live_instrument = "ADARA_FakeEvent"
        presenter.action_facility_changed(new_facility)

        self.mock_model.set_facility.assert_called_once_with(new_facility)
        self.mock_model.set_instrument.assert_called_once_with(default_test_live_instrument)

        self.assertEqual(presenter.get_view().instrument.getFacility(), "TEST_LIVE")

    def test_setup_confirmations(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        # check that the signals are connected to something
        self.assert_connected_once(presenter.get_view().prompt_save_on_close, presenter.get_view().prompt_save_on_close.stateChanged)

    def test_action_prompt_save_on_close(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_prompt_save_on_close(True)

        self.mock_model.set_prompt_save_on_close.assert_called_once_with(True)
        self.mock_model.set_prompt_save_on_close.reset_mock()

        presenter.action_prompt_save_on_close(False)

        self.mock_model.set_prompt_save_on_close.assert_called_once_with(False)

    def test_action_window_behaviour_changed(self):
        presenter = GeneralSettings(None, model=self.mock_model)
        values = presenter.WINDOW_BEHAVIOUR
        presenter.action_window_behaviour_changed(values[0])

        self.mock_model.set_window_behaviour.assert_called_once_with(values[0])
        self.mock_model.set_window_behaviour.reset_mock()

        presenter.action_window_behaviour_changed(values[1])

        self.mock_model.set_window_behaviour.assert_called_once_with(values[1])

    def test_action_prompt_save_editor_modified(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_prompt_save_editor_modified(True)

        self.mock_model.set_prompt_on_save_editor_modified.assert_called_once_with(True)
        self.mock_model.set_prompt_on_save_editor_modified.reset_mock()

        presenter.action_prompt_save_editor_modified(False)

        self.mock_model.set_prompt_on_save_editor_modified.assert_called_once_with(False)

    def test_action_prompt_deleting_workspace(self):
        presenter = GeneralSettings(None, model=self.mock_model)
        presenter.settings_presenter = MagicMock()

        presenter.action_prompt_deleting_workspace(True)

        self.mock_model.set_prompt_on_deleting_workspace.assert_called_once_with(True)
        self.mock_model.set_prompt_on_deleting_workspace.reset_mock()

        presenter.action_prompt_deleting_workspace(False)

        self.mock_model.set_prompt_on_deleting_workspace.assert_called_once_with(False)

    def test_load_current_setting_values(self):
        # load current setting is called automatically in the constructor
        GeneralSettings(None, model=self.mock_model)

        self.mock_model.get_prompt_save_on_close.assert_called_once()
        self.mock_model.get_prompt_on_save_editor_modified.assert_called_once()
        self.mock_model.get_prompt_on_deleting_workspace.assert_called_once()
        self.mock_model.get_project_recovery_enabled.assert_called_once()
        self.mock_model.get_project_recovery_time_between_recoveries.assert_called_once()
        self.mock_model.get_project_recovery_number_of_checkpoints.assert_called_once()
        self.mock_model.get_use_notifications.assert_called_once()
        self.mock_model.get_crystallography_convention.assert_called_once()
        self.mock_model.get_use_opengl.assert_called_once()
        self.mock_model.get_show_invisible_workspaces.assert_called_once()
        self.mock_model.get_completion_enabled.assert_called_once()

    def test_action_project_recovery_enabled(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_project_recovery_enabled(True)
        self.mock_model.set_project_recovery_enabled.assert_called_once_with("True")

        self.mock_model.set_project_recovery_enabled.reset_mock()

        presenter.action_project_recovery_enabled(False)
        self.mock_model.set_project_recovery_enabled.assert_called_once_with("False")

    def test_action_time_between_recovery(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        time = "6000"
        presenter.action_time_between_recovery(time)
        self.mock_model.set_project_recovery_time_between_recoveries.assert_called_once_with(time)

    def test_action_total_number_checkpoints(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        num_checkpoints = "532532"
        presenter.action_total_number_checkpoints(num_checkpoints)
        self.mock_model.set_project_recovery_number_of_checkpoints.assert_called_once_with(num_checkpoints)

    def test_action_instrument_changed(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        new_instr = "apples"
        presenter.action_instrument_changed(new_instr)
        self.mock_model.set_instrument(new_instr)

    def test_action_crystallography_convention(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_crystallography_convention(Qt.Checked)
        self.mock_model.set_crystallography_convention.assert_called_once_with("Crystallography")

        self.mock_model.set_crystallography_convention.reset_mock()

        presenter.action_crystallography_convention(Qt.Unchecked)
        self.mock_model.set_crystallography_convention.assert_called_once_with("Inelastic")

    def test_action_use_open_gl(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_use_open_gl(Qt.Checked)
        self.mock_model.set_use_opengl.assert_called_once_with("On")

        self.mock_model.set_use_opengl.reset_mock()

        presenter.action_use_open_gl(Qt.Unchecked)
        self.mock_model.set_use_opengl.assert_called_once_with("Off")

    def test_action_use_notifications_modified(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_use_notifications_modified(Qt.Checked)

        self.mock_model.set_use_notifications.assert_called_once_with("On")
        self.mock_model.set_use_notifications.reset_mock()

        presenter.action_use_notifications_modified(Qt.Unchecked)

        self.mock_model.set_use_notifications.assert_called_once_with("Off")

    def test_action_font_selected(self):
        presenter = GeneralSettings(None, model=self.mock_model)
        mock_font = Mock()
        mock_font.toString.return_value = "Serif"
        presenter.action_font_selected(mock_font)
        self.mock_model.set_font.assert_called_once_with("Serif")

    def test_action_show_invisible_workspaces(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_show_invisible_workspaces(True)
        self.mock_model.set_show_invisible_workspaces.assert_called_once_with("True")

        self.mock_model.set_show_invisible_workspaces.reset_mock()

        presenter.action_show_invisible_workspaces(False)
        self.mock_model.set_show_invisible_workspaces.assert_called_once_with("False")

    def test_action_completion_enabled_modified(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        presenter.action_completion_enabled_modified(True)
        self.mock_model.set_completion_enabled.assert_called_once_with(True)
        self.mock_model.set_completion_enabled.reset_mock()

        presenter.action_completion_enabled_modified(False)
        self.mock_model.set_completion_enabled.assert_called_once_with(False)

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_fill_layout_display(self, _):
        mock_view = Mock()
        presenter = GeneralSettings(None, view=mock_view, model=self.mock_model)

        test_dict = {"a": 1, "b": 2, "c": 3}
        self.mock_model.get_user_layout.return_value = test_dict
        # setup mock commands
        mock_view.layout_display.addItem = Mock()

        presenter.fill_layout_display()

        calls = [call("a"), call("b"), call("c")]
        mock_view.layout_display.addItem.assert_has_calls(calls)

    def test_get_layout_dict(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        test_dict = {"a": 1}
        self.mock_model.get_user_layout.return_value = test_dict

        self.assertEqual(test_dict, presenter.get_layout_dict())

    def test_get_layout_dict_key_error(self):
        presenter = GeneralSettings(None, model=self.mock_model)

        self.mock_model.get_user_layout.side_effect = KeyError()

        self.assertEqual({}, presenter.get_layout_dict())

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_save_layout(self, _):
        mock_view = Mock()
        presenter = GeneralSettings(None, view=mock_view, model=self.mock_model)
        # setup parent
        mock_parent = Mock()
        mock_parent.saveState.return_value = "value"
        presenter.parent = mock_parent

        test_dict = {"a": 1}
        self.mock_model.get_user_layout.return_value = test_dict
        # setup mock commands
        mock_view.new_layout_name.text = Mock(return_value="key")
        mock_view.new_layout_name.clear = Mock()

        presenter.save_layout()

        calls = [call(get_potential_update=True), call(get_potential_update=True)]
        self.mock_model.get_user_layout.assert_has_calls(calls)
        self.mock_model.set_user_layout.assert_called_once_with({"a": 1, "key": "value"})
        mock_parent.saveState.assert_called_once_with(SAVE_STATE_VERSION)
        mock_parent.populate_layout_menu.assert_called_once_with()

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_load_layout(self, _):
        mock_view = Mock()
        presenter = GeneralSettings(None, view=mock_view, model=self.mock_model)
        self.mock_model.get_user_layout.reset_mock()
        # setup parent
        mock_parent = Mock()
        presenter.parent = mock_parent
        # setup item selection
        list_item = Mock()
        list_item.text.return_value = "a"
        mock_view.layout_display.currentItem = Mock(return_value=list_item)

        test_dict = {"a": 1}
        self.mock_model.get_user_layout.return_value = test_dict

        presenter.load_layout()

        self.mock_model.get_user_layout.assert_called_once()
        mock_parent.restoreState.assert_called_once_with(test_dict["a"], SAVE_STATE_VERSION)

    @patch(MOUSEWHEEL_EVENT_FILTER_PATH)
    def test_delete_layout(self, _):
        mock_view = Mock()
        presenter = GeneralSettings(None, view=mock_view, model=self.mock_model)
        # setup parent
        mock_parent = Mock()
        presenter.parent = mock_parent
        # setup item selection
        list_item = Mock()
        list_item.text.return_value = "a"
        mock_view.layout_display.currentItem = Mock(return_value=list_item)

        test_dict = {"a": 1}
        self.mock_model.get_user_layout.return_value = test_dict

        presenter.delete_layout()

        calls = [call(get_potential_update=True), call(get_potential_update=True)]
        self.mock_model.get_user_layout.assert_has_calls(calls)
        self.mock_model.set_user_layout.assert_called_once_with({})
        mock_parent.populate_layout_menu.assert_called_once_with()
