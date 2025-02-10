# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import TestCase
from unittest.mock import MagicMock

from workbench.widgets.settings.base_classes.config_settings_presenter import SettingsPresenterBase


class ConfigSettingsPresenterTest(TestCase):
    def setUp(self) -> None:
        self.mock_model = MagicMock()
        self.mock_model.has_unsaved_changes = MagicMock()
        self.presenter = SettingsPresenterBase(self.mock_model)
        self._mock_parent_presenter = MagicMock()
        self.presenter.subscribe_parent_presenter(self._mock_parent_presenter)

    def test_notify_changes_with_changes(self):
        self.mock_model.has_unsaved_changes.return_value = True
        self.presenter.notify_changes()
        self._mock_parent_presenter.changes_updated.assert_called_once_with(True)

    def test_notify_changes_without_changes(self):
        self.mock_model.has_unsaved_changes.return_value = False
        self.presenter.notify_changes()
        self._mock_parent_presenter.changes_updated.assert_called_once_with(False)
