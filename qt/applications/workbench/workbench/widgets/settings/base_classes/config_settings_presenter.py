# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import Signal, QObject

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


class SettingsPresenterBase(QObject):
    unsaved_changes_signal = Signal(bool)

    def __init__(self, parent, model: ConfigSettingsChangesModel):
        super().__init__(parent)
        self._model: ConfigSettingsChangesModel = model

    def notify_changes(self):
        self.unsaved_changes_signal.emit(self._model.has_unsaved_changes())
