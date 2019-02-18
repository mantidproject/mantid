# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import, unicode_literals

from mock import Mock, call, patch

from mantidqt.utils.qt.test import GuiTest
from workbench.widgets.settings.general.presenter import GeneralSettings


class MockInstrument(object):
    def __init__(self, idx):
        self.name = Mock(return_value="instr{}".format(idx))


class MockFacility(object):
    all_instruments = [MockInstrument(0), MockInstrument(1)]

    name = Mock(return_value="MockFacility")
    instruments = Mock(return_value=all_instruments)


class MockConfigService(object):
    all_facilities = ["facility1", "facility2"]

    mock_facility = MockFacility()
    mock_instrument = MockFacility()

    getFacilityNames = Mock(return_value=all_facilities)
    getFacility = Mock(return_value=mock_facility)
    getInstrument = Mock(return_value=mock_instrument)
    getString = Mock(return_value="1")

    setFacility = Mock()
    setString = Mock()


class GeneralSettingsTest(GuiTest):

    def assert_connected_once(self, owner, signal):
        self.assertEqual(1, owner.receivers(signal))

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_setup_facilities(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        mock_ConfigService.setFacility.asset_called_once_with()
        mock_ConfigService.getFacility.asset_called_once_with()
        mock_ConfigService.mock_facility.name.asset_called_once_with()
        self.assert_connected_once(presenter.view.facility, presenter.view.facility.currentTextChanged)

        mock_ConfigService.getInstrument.asset_called_once_with()
        mock_ConfigService.mock_instrument.name.asset_called_once_with()
        self.assert_connected_once(presenter.view.instrument, presenter.view.instrument.currentTextChanged)

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
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

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_facility_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)

        new_facility = "WWW"
        presenter.action_facility_changed(new_facility)

        mock_ConfigService.setFacility.asset_called_once_with(new_facility)

        self.assertEqual(2, presenter.view.instrument.count())

    def test_setup_confirmations(self):
        presenter = GeneralSettings(None)

        # check that the signals are connected to something
        self.assert_connected_once(presenter.view.prompt_save_on_close,
                                   presenter.view.prompt_save_on_close.stateChanged)

        self.assert_connected_once(presenter.view.prompt_save_editor_modified,
                                   presenter.view.prompt_save_editor_modified.stateChanged)

    @patch("workbench.widgets.settings.general.presenter.CONF")
    def test_action_prompt_save_on_close(self, mock_conf):
        presenter = GeneralSettings(None)

        presenter.action_prompt_save_on_close(True)

        mock_conf.set.assert_called_once_with(GeneralSettings.PROJECT, GeneralSettings.PROMPT_SAVE_ON_CLOSE, True)
        mock_conf.set.reset_mock()

        presenter.action_prompt_save_on_close(False)

        mock_conf.set.assert_called_once_with(GeneralSettings.PROJECT, GeneralSettings.PROMPT_SAVE_ON_CLOSE, False)

    @patch("workbench.widgets.settings.general.presenter.CONF")
    def test_action_prompt_save_editor_modified(self, mock_CONF):
        presenter = GeneralSettings(None)

        presenter.action_prompt_save_on_close(True)

        mock_CONF.set.assert_called_once_with(GeneralSettings.PROJECT,
                                              GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED, True)
        mock_CONF.set.reset_mock()

        presenter.action_prompt_save_on_close(False)

        mock_CONF.set.assert_called_once_with(GeneralSettings.PROJECT,
                                              GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED, False)

    @patch("workbench.widgets.settings.general.presenter.CONF")
    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_load_current_setting_values(self, mock_ConfigService, mock_CONF):
        # load current setting is called automatically in the constructor
        GeneralSettings(None)

        # calls().__int__() are the calls to int() on the retrieved value from ConfigService.getString
        mock_CONF.get.assert_has_calls([call(GeneralSettings.PROJECT, GeneralSettings.PROMPT_SAVE_ON_CLOSE),
                                        call().__int__(),
                                        call(GeneralSettings.PROJECT, GeneralSettings.PROMPT_SAVE_EDITOR_MODIFIED),
                                        call().__int__()])

        mock_ConfigService.getString.assert_has_calls([call(GeneralSettings.PR_RECOVERY_ENABLED),
                                                       call(GeneralSettings.PR_TIME_BETWEEN_RECOVERY),
                                                       call(GeneralSettings.PR_NUMBER_OF_CHECKPOINTS)])

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_project_recovery_enabled(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_RECOVERY_ENABLED, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_project_recovery_enabled(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_RECOVERY_ENABLED, "False")

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_time_between_recovery(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        time = "6000"
        presenter.action_total_number_checkpoints(time)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_TIME_BETWEEN_RECOVERY, time)

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_total_number_checkpoints(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        num_checkpoints = "532532"
        presenter.action_total_number_checkpoints(num_checkpoints)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.PR_NUMBER_OF_CHECKPOINTS, num_checkpoints)

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_instrument_changed(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        new_instr = "apples"
        presenter.action_instrument_changed(new_instr)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.INSTRUMENT, new_instr)

    @patch("workbench.widgets.settings.general.presenter.ConfigService", new_callable=MockConfigService)
    def test_action_show_invisible_workspaces(self, mock_ConfigService):
        presenter = GeneralSettings(None)
        # reset any effects from the constructor
        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(True)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.SHOW_INVISIBLE_WORKSPACES, "True")

        mock_ConfigService.setString.reset_mock()

        presenter.action_show_invisible_workspaces(False)
        mock_ConfigService.setString.assert_called_once_with(GeneralSettings.SHOW_INVISIBLE_WORKSPACES, "False")
