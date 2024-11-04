# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from unittest import TestCase

from unittest.mock import call, MagicMock, Mock
from mantidqt.utils.testing.mocks.mock_qt import MockQButton, MockQWidget
from mantidqt.utils.testing.strict_mock import StrictPropertyMock
from workbench.widgets.settings.presenter import SettingsPresenter


class FakeMVP(object):
    def __init__(self):
        self.view = MockQWidget()
        self.update_properties = MagicMock()


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


class SettingsPresenterTest(TestCase):
    def test_default_view_shown(self):
        mock_view = MockSettingsView()
        SettingsPresenter(
            None,
            view=mock_view,
            general_settings=mock_view.general_settings,
            categories_settings=mock_view.categories_settings,
            plot_settings=mock_view.plot_settings,
            fitting_settings=mock_view.fitting_settings,
        )

        expected_calls = [call(mock_view.general_settings.view), call(mock_view.categories_settings.view)]
        mock_view.container.addWidget.assert_has_calls(expected_calls)

    def test_action_current_row_changed(self):
        mock_view = MockSettingsView()
        presenter = SettingsPresenter(
            None,
            view=mock_view,
            general_settings=mock_view.general_settings,
            categories_settings=mock_view.categories_settings,
            plot_settings=mock_view.plot_settings,
            fitting_settings=mock_view.fitting_settings,
        )

        mock_view.sections.item = Mock()
        mock_view.sections.item().text = Mock(return_value=presenter.SETTINGS_TABS["categories_settings"])
        presenter.action_section_changed(1)

        self.assertEqual(1, mock_view.container.replaceWidget.call_count)
        self.assertEqual(mock_view.categories_settings.view, presenter.current)

    def test_action_save_settings_to_file(self):
        mock_view = MockSettingsView()
        mock_model = MagicMock()
        presenter = SettingsPresenter(
            None,
            view=mock_view,
            model=mock_model,
            general_settings=mock_view.general_settings,
            categories_settings=mock_view.categories_settings,
            plot_settings=mock_view.plot_settings,
            fitting_settings=mock_view.fitting_settings,
        )

        presenter.action_save_settings_to_file()
        presenter.model.save_settings_to_file.assert_called_once_with("filename", presenter.all_properties)

    def test_action_load_settings_from_file(self):
        mock_view = MockSettingsView()
        mock_model = MagicMock()
        presenter = SettingsPresenter(
            None,
            view=mock_view,
            model=mock_model,
            general_settings=mock_view.general_settings,
            categories_settings=mock_view.categories_settings,
            plot_settings=mock_view.plot_settings,
            fitting_settings=mock_view.fitting_settings,
        )

        presenter.action_load_settings_from_file()
        presenter.model.load_settings_from_file.assert_called_once_with("filename", presenter.all_properties)
        presenter.general_settings.update_properties.assert_called_once_with()
        presenter.categories_settings.update_properties.assert_called_once_with()
        presenter.plot_settings.update_properties.assert_called_once_with()
        presenter.fitting_settings.update_properties.assert_called_once_with()

    def test_register_change_needs_restart(self):
        mock_view = MagicMock()
        mock_model = MagicMock()
        mock_parent = MagicMock()
        presenter = SettingsPresenter(
            mock_parent,
            view=mock_view,
            model=mock_model,
            general_settings=mock_view.general_settings,
            categories_settings=mock_view.categories_settings,
            plot_settings=mock_view.plot_settings,
            fitting_settings=mock_view.fitting_settings,
        )

        settings_needing_restart = ["Setting one", "Setting two"]
        for setting in settings_needing_restart:
            presenter.register_change_needs_restart(setting)
        presenter.view_closing()
        presenter.view.notify_changes_need_restart.assert_called_once_with(settings_needing_restart)
