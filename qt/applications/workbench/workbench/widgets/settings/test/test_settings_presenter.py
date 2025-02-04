# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import TestCase

from unittest.mock import call, MagicMock, Mock, patch
from mantidqt.utils.testing.mocks.mock_qt import MockQButton, MockQWidget
from mantidqt.utils.testing.strict_mock import StrictPropertyMock
from workbench.widgets.settings.presenter import SettingsPresenter


class FakeMVP(object):
    def __init__(self):
        self._view = MockQWidget()
        self.update_properties = MagicMock()
        self.get_view = MagicMock(return_value=self._view)
        self.unsaved_changes_signal = MagicMock()


class FakeSectionsListWidget:
    def __init__(self):
        self.fake_items = []
        self.setCurrentRow = MagicMock()

    def addItems(self, item):
        self.fake_items.extend(item)

    def item(self, index):
        return self.fake_items[index]


class MockSettingsView(object):
    def __init__(self):
        self.mock_container = MockQWidget()
        self.mock_current = MockQWidget()
        self.container = StrictPropertyMock(return_value=self.mock_container)
        self.current = StrictPropertyMock(return_value=self.mock_current)
        self.sections = FakeSectionsListWidget()
        self.general_settings = FakeMVP()
        self.categories_settings = FakeMVP()
        self.plot_settings = FakeMVP()
        self.fitting_settings = FakeMVP()
        self.save_settings_button = MockQButton()
        self.help_button = MockQButton()
        self.save_file_button = MockQButton()
        self.load_file_button = MockQButton()
        self.okay_button = MockQButton()
        self.apply_button = MockQButton()
        self.get_properties_filename = MagicMock(return_value="filename")
        self.notify_changes_need_restart = MagicMock()
        self.close = MagicMock()
        self.ask_before_close = MagicMock()


class SettingsPresenterTest(TestCase):
    def setUp(self) -> None:
        self.mock_view = MockSettingsView()
        self.mock_model = MagicMock()
        self.mock_parent = MagicMock()
        self.presenter = SettingsPresenter(
            self.mock_parent,
            view=self.mock_view,
            model=self.mock_model,
            general_settings=self.mock_view.general_settings,
            categories_settings=self.mock_view.categories_settings,
            plot_settings=self.mock_view.plot_settings,
            fitting_settings=self.mock_view.fitting_settings,
        )

    def test_default_view_shown(self):
        expected_calls = [call(self.mock_view.general_settings.get_view()), call(self.mock_view.categories_settings.get_view())]
        self.mock_view.container.addWidget.assert_has_calls(expected_calls)

    def test_action_current_row_changed(self):
        self.mock_view.sections.item = Mock()
        self.mock_view.sections.item().text = Mock(return_value=self.presenter.SETTINGS_TABS["categories_settings"])
        self.presenter.action_section_changed(1)

        self.assertEqual(1, self.mock_view.container.replaceWidget.call_count)
        self.assertEqual(self.mock_view.categories_settings.get_view(), self.presenter.current)

    def test_action_save_settings_to_file(self):
        self.presenter.action_save_settings_to_file()
        self.mock_model.save_settings_to_file.assert_called_once_with("filename", self.presenter.all_properties)

    def test_action_load_settings_from_file(self):
        self.presenter.action_load_settings_from_file()
        self.mock_model.load_settings_from_file.assert_called_once_with("filename", self.presenter.all_properties)
        self.presenter.general_settings.update_properties.assert_called_once_with()
        self.presenter.categories_settings.update_properties.assert_called_once_with()
        self.presenter.plot_settings.update_properties.assert_called_once_with()
        self.presenter.fitting_settings.update_properties.assert_called_once_with()

    def test_register_change_needs_restart(self):
        settings_needing_restart = ["Setting one", "Setting two"]
        self.mock_model.potential_changes_that_need_a_restart = MagicMock(return_value=settings_needing_restart)

        self.presenter.action_apply_button_pushed()
        self.presenter.view.notify_changes_need_restart.assert_called_once_with(settings_needing_restart)

    def test_update_apply_button(self):
        for changes in ({}, {"a": "change"}):
            self.mock_view.apply_button.setEnabled.reset_mock()
            self.mock_model.unsaved_changes.return_value = changes
            self.presenter.update_apply_button()
            self.mock_view.apply_button.setEnabled.assert_called_once_with(changes != {})

    def test_changes_updated(self):
        self.mock_view.apply_button.setEnabled.reset_mock()
        self.presenter.changes_updated(False)
        self.mock_view.apply_button.setEnabled.assert_called_once_with(False)
        self.mock_view.apply_button.setEnabled.reset_mock()
        self.presenter.changes_updated(True)
        self.mock_view.apply_button.setEnabled.assert_called_once_with(True)

    def test_presenter_signals_setup(self):
        self.mock_view.general_settings.unsaved_changes_signal.connect.assert_called_once_with(self.presenter.changes_updated)
        self.mock_view.plot_settings.unsaved_changes_signal.connect.assert_called_once_with(self.presenter.changes_updated)
        self.mock_view.fitting_settings.unsaved_changes_signal.connect.assert_called_once_with(self.presenter.changes_updated)
        self.mock_view.categories_settings.unsaved_changes_signal.connect.assert_called_once_with(self.presenter.changes_updated)

    @patch("workbench.widgets.settings.presenter.SettingsPresenter.update_apply_button")
    def test_action_apply_button_pushed(self, update_apply_mock: MagicMock):
        self.presenter.action_apply_button_pushed()
        self.mock_model.apply_all_settings.assert_called_once()
        self.mock_parent.config_updated.assert_called_once()
        update_apply_mock.assert_called_once()

    @patch("workbench.widgets.settings.presenter.SettingsPresenter.view_closing")
    def test_action_okay_button_pushed(self, mock_view_closing: MagicMock):
        self.presenter.action_okay_button_pushed()
        self.mock_model.apply_all_settings.assert_called_once()
        mock_view_closing.assert_called_once()

    @patch("workbench.widgets.settings.presenter.ConfigService")
    def test_view_closing_no_unsaved_changes(self, mock_config_service: MagicMock):
        self.mock_model.unsaved_changes.return_value = {}
        mock_config_service.getUserFilename.return_value = "username.file"
        closed = self.presenter.view_closing()

        self.assertTrue(closed)
        mock_config_service.saveConfig.assert_called_once_with("username.file")
        self.mock_parent.config_updated.assert_called_once()
        self.mock_view.close.assert_called_once()

    @patch("workbench.widgets.settings.presenter.ConfigService")
    def test_view_closing_unsaved_change_close_anyway(self, mock_config_service: MagicMock):
        self.mock_model.unsaved_changes.return_value = {"property": "change"}
        self.mock_view.ask_before_close.return_value = True
        mock_config_service.getUserFilename.return_value = "username.file"
        closed = self.presenter.view_closing()

        self.assertTrue(closed)
        mock_config_service.saveConfig.assert_called_once_with("username.file")
        self.mock_parent.config_updated.assert_called_once()
        self.mock_view.close.assert_called_once()

    def test_view_closing_unsaved_changes_choose_no(self):
        self.mock_model.unsaved_changes.return_value = {"property": "change"}
        self.mock_view.ask_before_close.return_value = False
        closed = self.presenter.view_closing()

        self.assertFalse(closed)
