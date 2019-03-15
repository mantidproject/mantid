# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

from mantid.py3compat import is_text_string
from posixpath import join as joinsettings
from qtpy.QtCore import QSettings


class UserConfig(object):
    """Holds user configuration option. Options are assigned a section
    and a key must only be unique within a section.

    Uses QSettings for the heavy lifting. All platforms use the Ini format
    at UserScope
    """

    # The raw QSettings instance
    qsettings = None

    def __init__(self, organization, application, defaults=None):
        """
        :param organization: A string name for the organization
        :param application: A string name for the application name
        :param defaults: Default configuration values for this instance in the
        form of nested dict instances
        """
        # Loads the saved settings if found
        self.qsettings = QSettings(QSettings.IniFormat, QSettings.UserScope,
                                   organization, application)

        # convert the defaults into something that qsettings can handle
        default_settings = self._flatten_defaults(defaults)

        # put defaults into qsettings if they weren't there already
        try:
            self.set_qsettings_values(default_settings)
        # the editors/sessiontabs are pickled in config so need to remove them
        except ValueError:
            self.qsettings.remove('Editors/SessionTabs')
            self.set_qsettings_values(default_settings)

        # fixup the values of booleans - they do not evaluate correctly when read from the config file
        for key in self.all_keys():
            try:
                value = self.get(key)
            except KeyError:
                continue
            if value == 'true':
                self.set(key, True)
            elif value == 'false':
                self.set(key, False)

    def set_qsettings_values(self, default_settings):
        configFileKeys = self.qsettings.allKeys()
        for key in default_settings.keys():
            if key not in configFileKeys:
                self.qsettings.setValue(key, default_settings[key])

    def all_keys(self, group=None):
        if group is not None:
            self.qsettings.beginGroup(group)
            result = self.qsettings.allKeys()
            self.qsettings.endGroup()
        else:
            result = self.qsettings.allKeys()

        return result

    @property
    def filename(self):
        return self.qsettings.fileName()

    def get(self, option, second=None):
        """Return a value for an option. If two arguments are given the first
        is the group/section and the second is the option within it.
        ``config.get('main', 'window/size')`` is equivalent to
        ``config.get('main/window/size')`` If no option is found then
        a KeyError is raised
        """
        option = self._check_section_option_is_valid(option, second)
        value = self.qsettings.value(option)

        # qsettings appears to return None if the option isn't found
        if value is None:
            raise KeyError('Unknown config item requested: "{}"'.format(option))
        else:
            return value

    def has(self, option, second=None):
        """Return a True if the key exists in the
        settings. ``config.get('main', 'window/size')`` and
        ``config.get('main/window/size')`` are equivalent.
        """
        option = self._check_section_option_is_valid(option, second)
        return option in self.all_keys()

    def set(self, option, value, extra=None):
        """Set a value for an option in a given section. Can either supply
        the fully qualified option or add the section as an additional
        first argument. ``config.set('main', 'high_dpi_scaling',
        True)`` is equivalent to ``config.set('main/high_dpi_scaling',
        True)``
        """
        if extra is None:
            option = self._check_section_option_is_valid(option, extra)
            # value is in the right place
        else:
            option = self._check_section_option_is_valid(option, value)
            value = extra
        self.qsettings.setValue(option, value)

    def remove(self, option, second=None):
        """Removes a key from the settings. Key not existing returns without effect.
        """
        option = self._check_section_option_is_valid(option, second)
        if self.has(option):
            self.qsettings.remove(option)

    # -------------------------------------------------------------------------
    # "Private" methods
    # -------------------------------------------------------------------------

    @staticmethod
    def _flatten_defaults(input_dict):
        result = {}
        for key in input_dict:
            value = input_dict[key]
            if isinstance(value, dict):
                value = UserConfig._flatten_defaults(value)
                for key_inner in value.keys():
                    result[joinsettings(key, key_inner)] = value[key_inner]
            else:
                result[key] = value
        return result

    def _check_section_option_is_valid(self, option, second):
        """
        Sanity check the section and option are strings and return the flattened option key
        """
        if second is None:
            if not is_text_string(option):
                raise TypeError('Found invalid type ({}) for option ({}) must be a string'.format(type(option), option))
            return option
        else: # fist argument is actually the section/group
            if not is_text_string(option):
                raise TypeError('Found invalid type ({}) for section ({}) must be a string'.format(type(option), option))
            if not is_text_string(second):
                raise TypeError('Found invalid type ({}) for option ({}) must be a string'.format(type(second), second))
            return joinsettings(option, second)
