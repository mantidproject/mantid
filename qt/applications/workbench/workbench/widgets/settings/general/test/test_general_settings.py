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
from workbench.widgets.settings.general.presenter import GeneralSettings, GeneralProperties
from qtpy.QtCore import Qt


class MockInstrument(object):
    def __init__(self, idx):
        self.name = StrictMock(return_value="instr{}".format(idx))


class MockFacility(object):
    def __init__(self, name):
        self.name = StrictMock(return_value=name)
        self.all_instruments = [MockInstrument(0), MockInstrument(1)]
        self.instruments = StrictMock(return_value=self.all_instruments)


class MockConfigService(object):
    all_facilities = ["facility1", "facility2"]

    def __init__(self):
        self.mock_facility = MockFacility(self.all_facilities[0])
        self.mock_instrument = self.mock_facility.all_instruments[0]
        self.getFacilityNames = StrictMock(return_value=self.all_facilities)
        self.getFacility = StrictMock(return_value=self.mock_facility)
        self.getInstrument = StrictMock(return_value=self.mock_instrument)
        self.getString = StrictMock(return_value="1")
        self.setFacility = StrictMock()
        self.setString = StrictMock()


@start_qapplication
class GeneralSettingsTest(unittest.TestCase):
    CONFIG_SERVICE_CLASSPATH = "workbench.widgets.settings.general.presenter.ConfigService"
    WORKBENCH_CONF_CLASSPATH = "workbench.widgets.settings.general.presenter.CONF"

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_valid_combination(self, mock_ConfigService):
        self.assertEqual(0, mock_ConfigService.mock_instrument.name.call_count)
        presenter = GeneralSettings(None)
        self.assertEqual(0, mock_ConfigService.setFacility.call_count)
        self.assertEqual(2, mock_ConfigService.getFacility.call_count)
        self.assertEqual(2, mock_ConfigService.mock_facility.name.call_count)
        self.assert_connected_once(presenter.view.facility, presenter.view.facility.currentTextChanged)

        mock_ConfigService.getInstrument.assert_called_once_with()
        self.assertEqual(1, mock_ConfigService.mock_instrument.name.call_count)
        self.assert_connected_once(presenter.view.instrument, presenter.view.instrument.currentTextChanged)

    def test_setup_checkbox_signals(self):
        presenter = GeneralSettings(None)

        self.assert_connected_once(presenter.view.crystallography_convention,
                                   presenter.view.crystallography_convention.stateChanged)

        self.assert_connected_once(presenter.view.use_open_gl,
                                   presenter.view.use_open_gl.stateChanged)

        self.assert_connected_once(presenter.view.show_invisible_workspaces,
                                   presenter.view.show_invisible_workspaces.stateChanged)

        self.assert_connected_once(presenter.view.project_recovery_enabled,
                                   presenter.view.project_recovery_enabled.stateChanged)

        self.assert_connected_once(presenter.view.time_between_recovery,
                                   presenter.view.time_between_recovery.valueChanged)

        self.assert_connected_once(presenter.view.total_number_checkpoints,
                                   presenter.view.total_number_checkpoints.valueChanged)

    def test_font_dialog_signals(self):
        presenter = GeneralSettings(None)
        with patch.object(presenter.view, 'create_font_dialog', MagicMock()) as font_dialog:
            presenter.action_main_font_button_clicked()
            self.assertEqual(1, font_dialog().fontSelected.connect.call_count)

    def test_setup_general_group_signals(self):
        presenter = GeneralSettings(None)

        self.assert_connected_once(presenter.view.main_font,
                                   presenter.view.main_font.clicked)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_facility_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        mock_ConfigService.setFacility.reset_mock()

        new_facility = "TEST_LIVE"
        presenter.action_facility_changed(new_facility)

        mock_ConfigService.setFacility.assert_called_once_with(new_facility)

        self.assertEqual(43, presenter.view.instrument.count())

    def test_setup_confirmations(self):
        presenter = GeneralSettings(None)

        # check that the signals are connected to something
        self.assert_connected_once(presenter.view.prompt_save_on_close,
                                   presenter.view.prompt_save_on_close.stateChanged)

        self.assert_connected_once(presenter.view.prompt_save_editor_modified,
                                   presenter.view.prompt_save_editor_modified.stateChanged)

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_action_prompt_save_on_close(self, mock_conf):
        presenter = GeneralSettings(None)

        presenter.action_prompt_save_on_close(True)

        mock_conf.set.assert_called_once_with(GeneralProperties.PROMPT_SAVE_ON_CLOSE.value, True)
        mock_conf.set.reset_mock()

        presenter.action_prompt_save_on_close(False)

        mock_conf.set.assert_called_once_with(GeneralProperties.PROMPT_SAVE_ON_CLOSE.value, False)

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_action_prompt_save_editor_modified(self, mock_CONF):
        presenter = GeneralSettings(None)

        presenter.action_prompt_save_editor_modified(True)

        mock_CONF.set.assert_called_once_with(GeneralProperties.PROMPT_SAVE_EDITOR_MODIFIED.value, True)
        mock_CONF.set.reset_mock()

        presenter.action_prompt_save_editor_modified(False)

        mock_CONF.set.assert_called_once_with(GeneralProperties.PROMPT_SAVE_EDITOR_MODIFIED.value, False)

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_action_prompt_deleting_workspace(self, mock_CONF):
        presenter = GeneralSettings(None)
        presenter.settings_presenter = MagicMock()

        presenter.action_prompt_deleting_workspace(True)

        mock_CONF.set.assert_called_once_with(GeneralProperties.PROMPT_ON_DELETING_WORKSPACE.value, True)
        presenter.settings_presenter.register_change_needs_restart.assert_called_once()
        mock_CONF.set.reset_mock()
        presenter.settings_presenter.reset_mock()

        presenter.action_prompt_deleting_workspace(False)

        mock_CONF.set.assert_called_once_with(GeneralProperties.PROMPT_ON_DELETING_WORKSPACE.value, False)
        presenter.settings_presenter.register_change_needs_restart.assert_called_once()

    @patch(WORKBENCH_CONF_CLASSPATH)
    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_load_current_setting_values(self, mock_ConfigService, mock_CONF):
        # load current setting is called automatically in the constructor
        GeneralSettings(None)

        # calls().__bool__() are the calls to bool() on the retrieved value from ConfigService.getString
        mock_CONF.get.assert_has_calls([call(GeneralProperties.PROMPT_SAVE_ON_CLOSE.value),
                                        call().__bool__(),
                                        call(GeneralProperties.PROMPT_SAVE_EDITOR_MODIFIED.value),
                                        call().__bool__()])

        mock_ConfigService.getString.assert_has_calls([call(GeneralProperties.PR_RECOVERY_ENABLED.value),
                                                       call(GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value),
                                                       call(GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value),
                                                       call(GeneralProperties.USE_NOTIFICATIONS.value),
                                                       call(GeneralProperties.CRYSTALLOGRAPY_CONV.value),
                                                       call(GeneralProperties.OPENGL.value)])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_project_recovery_enabled(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.PR_RECOVERY_ENABLED.value, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.PR_RECOVERY_ENABLED.value, "False")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_time_between_recovery(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        time = "6000"
        presenter.action_time_between_recovery(time)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.PR_TIME_BETWEEN_RECOVERY.value, time)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_total_number_checkpoints(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        num_checkpoints = "532532"
        presenter.action_total_number_checkpoints(num_checkpoints)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.PR_NUMBER_OF_CHECKPOINTS.value,
                                                             num_checkpoints)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_instrument_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        new_instr = "apples"
        presenter.action_instrument_changed(new_instr)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.INSTRUMENT.value, new_instr)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_crystallography_convention(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_crystallography_convention(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.CRYSTALLOGRAPY_CONV.value,
                                                             "Crystallography")

        mock_ConfigService.setString.reset_mock()

        presenter.action_crystallography_convention(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.CRYSTALLOGRAPY_CONV.value, "Inelastic")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_use_open_gl(self, mock_ConfigService):
        presenter = GeneralSettings(None, view=Mock())
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_use_open_gl(Qt.Checked)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.OPENGL.value, "On")

        mock_ConfigService.setString.reset_mock()

        presenter.action_use_open_gl(Qt.Unchecked)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.OPENGL.value, "Off")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_use_notifications_modified(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        mock_ConfigService.setString.reset_mock()

        presenter.action_use_notifications_modified(Qt.Checked)

        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.USE_NOTIFICATIONS.value, "On")
        mock_ConfigService.setString.reset_mock()

        presenter.action_use_notifications_modified(Qt.Unchecked)

        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.USE_NOTIFICATIONS.value, "Off")

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_action_font_selected(self, mock_conf):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_conf.set.reset_mock()
        mock_font = Mock()
        mock_font.toString.return_value = "Serif"
        presenter.action_font_selected(mock_font)
        mock_conf.set.assert_called_once_with(GeneralProperties.FONT.value, "Serif")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_invisible_workspaces(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralProperties.SHOW_INVISIBLE_WORKSPACES.value, "False")

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_fill_layout_display(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get returns dictionary
        test_dict = {'a': 1, 'b': 2, 'c': 3}
        mock_CONF.get.return_value = test_dict
        # setup mock commands
        presenter.view.layout_display.addItem = Mock()

        presenter.fill_layout_display()

        calls = [call('a'), call('b'), call('c')]
        presenter.view.layout_display.addItem.assert_has_calls(calls)

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_get_layout_dict(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get returns dictionary
        test_dict = {'a': 1}
        mock_CONF.get.return_value = test_dict

        self.assertEqual(test_dict, presenter.get_layout_dict())

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_get_layout_dict_key_error(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get to return KeyError
        mock_CONF.get.side_effect = KeyError()

        self.assertEqual({}, presenter.get_layout_dict())

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_save_layout(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup parent
        mock_parent = Mock()
        mock_parent.saveState.return_value = "value"
        presenter.parent = mock_parent
        # setup CONF.get returns dictionary
        test_dict = {'a': 1}
        mock_CONF.get = Mock(return_value=test_dict)
        # setup mock commands
        presenter.view.new_layout_name.text = Mock(return_value='key')
        presenter.view.new_layout_name.clear = Mock()

        presenter.save_layout()

        calls = [call(GeneralProperties.USER_LAYOUT.value), call(GeneralProperties.USER_LAYOUT.value)]
        mock_CONF.get.assert_has_calls(calls)
        mock_parent.saveState.assert_called_once_with()
        mock_parent.populate_layout_menu.assert_called_once_with()

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_load_layout(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup parent
        mock_parent = Mock()
        presenter.parent = mock_parent
        # setup item selection
        list_item = Mock()
        list_item.text.return_value = 'a'
        presenter.view.layout_display.currentItem = Mock(return_value=list_item)
        # setup CONF.get returns dictionary
        test_dict = {'a': 1}
        mock_CONF.get = Mock(return_value=test_dict)

        presenter.load_layout()

        mock_CONF.get.assert_called_once_with(GeneralProperties.USER_LAYOUT.value)
        mock_parent.restoreState.assert_called_once_with(test_dict['a'])

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_delete_layout(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup parent
        mock_parent = Mock()
        presenter.parent = mock_parent
        # setup item selection
        list_item = Mock()
        list_item.text.return_value = 'a'
        presenter.view.layout_display.currentItem = Mock(return_value=list_item)
        # setup CONF.get returns dictionary
        test_dict = {'a': 1}
        mock_CONF.get = Mock(return_value=test_dict)

        presenter.delete_layout()

        calls = [call(GeneralProperties.USER_LAYOUT.value), call(GeneralProperties.USER_LAYOUT.value)]
        mock_CONF.get.assert_has_calls(calls)
        mock_CONF.set.assert_called_once_with(GeneralProperties.USER_LAYOUT.value, {})
        mock_parent.populate_layout_menu.assert_called_once_with()
