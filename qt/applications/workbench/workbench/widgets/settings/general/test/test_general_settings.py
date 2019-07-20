# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

import unittest

from mantid.py3compat.mock import call, patch, Mock
from mantidqt.utils.qt.testing import start_qapplication
from mantidqt.utils.testing.strict_mock import StrictMock
from workbench.widgets.settings.general.presenter import GeneralSettings


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
        mock_ConfigService.setFacility.assert_called_once_with(mock_ConfigService.mock_facility.name())
        self.assertEqual(2, mock_ConfigService.getFacility.call_count)
        self.assertEqual(2, mock_ConfigService.mock_facility.name.call_count)
        self.assert_connected_once(presenter.view.facility, presenter.view.facility.currentTextChanged)

        mock_ConfigService.getInstrument.assert_called_once_with()
        self.assertEqual(2, mock_ConfigService.mock_instrument.name.call_count)
        self.assert_connected_once(presenter.view.instrument, presenter.view.instrument.currentTextChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_invalid_default_facility_chooses_first(self, mock_ConfigService):
        mock_ConfigService.getFacility.side_effect = [RuntimeError("Invalid facility name"), mock_ConfigService.mock_facility]
        presenter = GeneralSettings(None)

        self.assertEqual(mock_ConfigService.mock_facility.name(),
                         presenter.view.facility.currentText())
        self.assertEqual(mock_ConfigService.mock_instrument.name(),
                         presenter.view.instrument.currentText())
        self.assert_connected_once(presenter.view.facility, presenter.view.facility.currentTextChanged)
        self.assert_connected_once(presenter.view.instrument, presenter.view.instrument.currentTextChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_facilities_with_invalid_default_instrument_chooses_first(self, mock_ConfigService):
        mock_ConfigService.getInstrument.side_effect = [RuntimeError("Invalid instrument name"),
                                                        mock_ConfigService.mock_instrument]
        presenter = GeneralSettings(None)

        self.assertEqual(mock_ConfigService.mock_instrument.name(),
                         presenter.view.instrument.currentText())
        self.assert_connected_once(presenter.view.facility, presenter.view.facility.currentTextChanged)
        self.assert_connected_once(presenter.view.instrument, presenter.view.instrument.currentTextChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_setup_checkbox_signals(self, _):
        presenter = GeneralSettings(None)

        self.assert_connected_once(presenter.view.show_invisible_workspaces,
                                   presenter.view.show_invisible_workspaces.stateChanged)

        self.assert_connected_once(presenter.view.project_recovery_enabled,
                                   presenter.view.project_recovery_enabled.stateChanged)

        self.assert_connected_once(presenter.view.time_between_recovery,
                                   presenter.view.time_between_recovery.valueChanged)

        self.assert_connected_once(presenter.view.total_number_checkpoints,
                                   presenter.view.total_number_checkpoints.valueChanged)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_facility_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        mock_ConfigService.setFacility.reset_mock()

        new_facility = "WWW"
        presenter.action_facility_changed(new_facility)

        mock_ConfigService.setFacility.assert_called_once_with(new_facility)

        self.assertEqual(2, presenter.view.instrument.count())

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

        mock_conf.set.assert_called_once_with(GeneralSettings.PROMPT_SAVE_ON_CLOSE, True)
        mock_conf.set.reset_mock()

        presenter.action_prompt_save_on_close(False)

        mock_conf.set.assert_called_once_with(GeneralSettings.PROMPT_SAVE_ON_CLOSE, False)

    @patch(WORKBENCH_CONF_CLASSPATH)
    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_prompt_save_editor_modified(self, mock_ConfigService, mock_CONF):
        presenter = GeneralSettings(None)

        presenter.action_prompt_save_editor_modified(True)

        mock_CONF.set.assert_called_once_with(GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED, True)
        mock_CONF.set.reset_mock()

        presenter.action_prompt_save_editor_modified(False)

        mock_CONF.set.assert_called_once_with(GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED, False)

    @patch(WORKBENCH_CONF_CLASSPATH)
    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_load_current_setting_values(self, mock_ConfigService, mock_CONF):
        # load current setting is called automatically in the constructor
        GeneralSettings(None)

        # calls().__int__() are the calls to int() on the retrieved value from ConfigService.getString
        mock_CONF.get.assert_has_calls([call(GeneralSettings.PROMPT_SAVE_ON_CLOSE),
                                        call().__int__(),
                                        call(GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED),
                                        call().__int__()])

        mock_ConfigService.getString.assert_has_calls([call(GeneralSettings.PR_RECOVERY_ENABLED),
                                                       call(GeneralSettings.PR_TIME_BETWEEN_RECOVERY),
                                                       call(GeneralSettings.PR_NUMBER_OF_CHECKPOINTS)])

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_project_recovery_enabled(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_RECOVERY_ENABLED, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_RECOVERY_ENABLED, "False")

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_time_between_recovery(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        time = "6000"
        presenter.action_time_between_recovery(time)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_TIME_BETWEEN_RECOVERY, time)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_total_number_checkpoints(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        num_checkpoints = "532532"
        presenter.action_total_number_checkpoints(num_checkpoints)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_NUMBER_OF_CHECKPOINTS, num_checkpoints)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_instrument_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        new_instr = "apples"
        presenter.action_instrument_changed(new_instr)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.INSTRUMENT, new_instr)

    @patch(CONFIG_SERVICE_CLASSPATH, new_callable=MockConfigService)
    def test_action_show_invisible_workspaces(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.SHOW_INVISIBLE_WORKSPACES, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.SHOW_INVISIBLE_WORKSPACES, "False")

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_fill_layout_display(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get returns dictionary
        test_dict = {'a': 1, 'b': 2, 'c': 3}
        mock_CONF.get.return_value = test_dict
        # setup mock commands
        presenter.view.layout_display.addItem = Mock()

        presenter.fill_layout_display()

        calls = [call('a'), call('b'),  call('c')]
        presenter.view.layout_display.addItem.assert_has_calls(calls)

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_get_layout_dict(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get returns dictionary
        test_dict = {'a': 1}
        mock_CONF.get.return_value = test_dict

        self.assertEquals(test_dict, presenter.get_layout_dict())

    @patch(WORKBENCH_CONF_CLASSPATH)
    def test_get_layout_dict_key_error(self, mock_CONF):
        presenter = GeneralSettings(None, view=Mock())
        # setup CONF.get to return KeyError
        mock_CONF.get.side_effect = KeyError()

        self.assertEquals({}, presenter.get_layout_dict())

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

        calls = [call(presenter.USER_LAYOUT), call(presenter.USER_LAYOUT)]
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

        mock_CONF.get.assert_called_once_with(presenter.USER_LAYOUT)
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

        calls = [call(presenter.USER_LAYOUT), call(presenter.USER_LAYOUT)]
        mock_CONF.get.assert_has_calls(calls)
        mock_CONF.set.assert_called_once_with(presenter.USER_LAYOUT, {})
        mock_parent.populate_layout_menu.assert_called_once_with()
