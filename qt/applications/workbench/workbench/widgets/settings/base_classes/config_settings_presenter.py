# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from abc import ABC

from workbench.widgets.settings.base_classes.config_settings_changes_model import ConfigSettingsChangesModel


class SettingsPresenterBase(ABC):
    def __init__(self, model: ConfigSettingsChangesModel):
        self._model: ConfigSettingsChangesModel = model
        self._parent_presenter = None

    def notify_changes(self):
        if self._parent_presenter is not None:
            self._parent_presenter.changes_updated(self._model.has_unsaved_changes())

    def subscribe_parent_presenter(self, parent_presenter):
        self._parent_presenter = parent_presenter
