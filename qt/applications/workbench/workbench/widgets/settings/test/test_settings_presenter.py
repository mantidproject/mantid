# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
from __future__ import absolute_import, unicode_literals

from unittest import TestCase

from mantidqt.utils.testing.mocks.mock_qt import MockQButton, MockQWidget
from mantidqt.utils.testing.strict_mock import StrictPropertyMock
from workbench.widgets.settings.presenter import SettingsPresenter


class FakeMVP(object):
    def __init__(self):
        self.view = MockQWidget()


class MockSettingsView(object):
    def __init__(self):
        self.mock_container = MockQWidget()
        self.mock_current = MockQWidget()
        self.container = StrictPropertyMock(return_value=self.mock_container)
        self.current = StrictPropertyMock(return_value=self.mock_current)
        self.general_settings = FakeMVP()

        self.save_settings_button = MockQButton()


class SettingsPresenterTest(TestCase):
    def test_default_view_shown(self):
        mock_view = MockSettingsView()
        SettingsPresenter(None, view=mock_view, general_settings=mock_view.general_settings)

        mock_view.container.addWidget.assert_called_once_with(mock_view.general_settings.view)

    def test_action_current_row_changed(self):
        mock_view = MockSettingsView()
        p = SettingsPresenter(None, view=mock_view, general_settings=mock_view.general_settings)

        p.action_section_changed(0)

        # Currently this is not called, because we only have 1 view.
        # When more views are added this test WILL BREAK, and should be adapted
        # to check if the views are being switched correctly
        self.assertEqual(0, mock_view.container.replaceWidget.call_count)
