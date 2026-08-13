# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

"""A QSettings facade that avoids ineffective setValue and remove calls."""

from qtpy.QtCore import QSettings


class QSettingsChangeAware:
    """Apply only effective changes to an owned or supplied QSettings instance."""

    def __init__(self, settings=None):
        self._settings = QSettings() if settings is None else settings
        self._changed = False

    @property
    def changed(self):
        """Whether this facade has invoked a mutating QSettings operation."""
        return self._changed

    def setValue(self, key, value, normalizer=None):
        """Set *key* only when its effective value differs from *value*."""
        if self._settings.contains(key):
            current = self._settings.value(key)
            if normalizer is not None:
                equal = normalizer(current) == normalizer(value)
            else:
                current = self._value_converted_to_requested_type(key, value, current)
                equal = current == value
            if equal:
                return False

        self._settings.setValue(key, value)
        self._changed = True
        return True

    def remove(self, key):
        """Remove *key* or its descendants only when either exists."""
        if not self._contains_key_or_children(key):
            return False

        self._settings.remove(key)
        self._changed = True
        return True

    def _value_converted_to_requested_type(self, key, requested, fallback):
        if requested is None:
            return fallback
        try:
            return self._settings.value(key, type=type(requested))
        except (TypeError, ValueError):
            return fallback

    def _contains_key_or_children(self, key):
        if self._settings.contains(key):
            return True
        if not key:
            return bool(self._settings.allKeys())

        self._settings.beginGroup(key)
        try:
            return bool(self._settings.allKeys())
        finally:
            self._settings.endGroup()
