# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
from __future__ import absolute_import, unicode_literals

from unittest import TestCase

from mock import Mock
from qtpy.QtWidgets import QWidget

from workbench.widgets.settings.presenter import SettingsPresenter


class FakeMVP(object):
    def __init__(self):
        self.view = Mock()


class MockSettingsView(object):
    def __init__(self):
        self.mock_container = Mock(QWidget)
        self.container = Mock(return_value=self.mock_container)
        self.mock_current = Mock(QWidget)
        self.current = self.mock_current
        self.general_settings = FakeMVP()


class SettingsPresenterTest(TestCase):
    def test_action_current_row_changed(self):
        mock_view = MockSettingsView()
        p = SettingsPresenter(None, view=mock_view)

        p.action_current_row_changed(0)

        mock_view.container.replaceWidget.assert_called_once_with(mock_view.mock_current,
                                                                  mock_view.general_settings.view)

        # check that the current view has been correctly reassigned
        self.assertEqual(mock_view.current, mock_view.general_settings.view)
