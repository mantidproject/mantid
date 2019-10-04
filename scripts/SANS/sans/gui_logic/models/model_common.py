# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
""" The settings diagnostic tab which visualizes the SANS state object. """
from __future__ import (absolute_import, division, print_function)

from abc import ABCMeta
from six import with_metaclass

from sans.common.enums import SANSInstrument
from sans.user_file.settings_tags import MonId, monitor_spectrum, DetectorId


class ModelCommon(with_metaclass(ABCMeta)):
    def __init__(self, user_file_items):
        # Workaround to avoid refactoring becoming impossibly large
        # TODO this should be encapsulated in sub-models
        if user_file_items is None:
            # Cannot iterate a None type when doing lookups
            self._user_file_items = {}
        else:
            self._user_file_items = user_file_items

    @property
    def instrument(self):
        return self.get_simple_element(element_id=DetectorId.instrument, default_value=SANSInstrument.NoInstrument)

    @instrument.setter
    def instrument(self, value):
        self.set_simple_element(element_id=DetectorId.instrument, value=value)

    def _get_incident_spectrum_info(self, default_value, attribute, is_trans):
        if MonId.spectrum in self._user_file_items:
            settings = self._user_file_items[MonId.spectrum]
            if is_trans:
                settings = [setting for setting in settings if setting.is_trans]
            else:
                settings = [setting for setting in settings if not setting.is_trans]
            element = settings[-1]
            return getattr(element, attribute)
        else:
            return default_value

    def _update_incident_spectrum_info(self, spectrum=None, interpolate=False, is_trans=False):
        if MonId.spectrum in self._user_file_items:
            settings = self._user_file_items[MonId.spectrum]
        else:
            # If the entry does not already exist, then add it.
            settings = [monitor_spectrum(spectrum=spectrum, is_trans=is_trans, interpolate=interpolate)]

        new_settings = []
        for setting in settings:
            # Only modify the settings which match the is_trans, selection. Else we muddle up the normalize to monitor
            # settings with the transmission settings.
            if setting.is_trans == is_trans:
                new_spectrum = spectrum if spectrum else setting.spectrum
                new_interpolate = interpolate if interpolate else setting.interpolate
                new_setting = monitor_spectrum(spectrum=new_spectrum, is_trans=is_trans,
                                               interpolate=new_interpolate)
                new_settings.append(new_setting)
            else:
                new_settings.append(setting)
        self._user_file_items.update({MonId.spectrum: new_settings})

    def get_simple_element(self, element_id, default_value):
        return self.get_simple_element_with_attribute(element_id, default_value)

    def set_simple_element(self, element_id, value):
        if element_id in self._user_file_items:
            del self._user_file_items[element_id]
        new_state_entries = {element_id: [value]}
        self._user_file_items.update(new_state_entries)

    def get_simple_element_with_attribute(self, element_id, default_value, attribute=None):
        if element_id in self._user_file_items:
            element = self._user_file_items[element_id][-1]
            return getattr(element, attribute) if attribute else element
        else:
            return default_value
