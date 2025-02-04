# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from unittest import TestCase
from unittest.mock import MagicMock, patch

from workbench.widgets.settings.base_classes.config_settings_presenter import SettingsPresenterBase


class ConfigSettingsPresenterTest(TestCase):
    UNSAVED_CHANGES_SIGNAL_PATH = (
        "workbench.widgets.settings.base_classes.config_settings_presenter.SettingsPresenterBase.unsaved_changes_signal"
    )

    def setUp(self) -> None:
        self.mock_model = MagicMock()
        self.mock_model.has_unsaved_changes = MagicMock()
        self.presenter = SettingsPresenterBase(None, self.mock_model)

    @patch(UNSAVED_CHANGES_SIGNAL_PATH)
    def test_notify_changes_with_changes(self, mock_signal: MagicMock):
        self.mock_model.has_unsaved_changes.return_value = True
        self.presenter.notify_changes()
        mock_signal.emit.assert_called_once_with(True)

    @patch(UNSAVED_CHANGES_SIGNAL_PATH)
    def test_notify_changes_without_changes(self, mock_signal: MagicMock):
        self.mock_model.has_unsaved_changes.return_value = False
        self.presenter.notify_changes()
        mock_signal.emit.assert_called_once_with(False)
