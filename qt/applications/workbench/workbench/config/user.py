#  This file is part of the mantid workbench.
#
#  Copyright (C) 2017 mantidproject
#
#  This program is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <http://www.gnu.org/licenses/>.
from __future__ import (absolute_import, division, print_function,
                        unicode_literals)

from mantidqt.py3compat import is_text_string

from qtpy.QtCore import QSettings


class UserConfig(object):
    """Holds user configuration option. Options are assigned a section
    and a key must only be unique within a section.

    Uses QSettings for the heavy lifting. All platforms use the Ini format
    at UserScope
    """

    # The raw QSettings instance
    qsettings = None
    defaults = None

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
        self.defaults = defaults

    def all_keys(self):
        return self.qsettings.allKeys()

    @property
    def filename(self):
        return self.qsettings.fileName()

    def get(self, section, option):
        """
        Return a value for an option in a given section. If not
        specified in the saved settings then the initial
        defaults are consulted. If no option is found then
        a KeyError is raised
        :param section: A string section name
        :param option: A string option name
        :return: The value of the option
        """
        value = self.qsettings.value(self._settings_path(section, option))
        if not value:
            value = self._get_default_or_raise(section, option)

        return value

    def set(self, section, option, value):
        """
        Set a value for an option in a given section.
        :param section: A string section name
        :param option: A string option name
        :param value: The value of the setting
        """
        self.qsettings.setValue(self._settings_path(section, option), value)

    # -------------------------------------------------------------------------
    # "Private" methods
    # -------------------------------------------------------------------------

    def _check_section_option_is_valid(self, section, option):
        """
        Sanity check the section and option are strings
        """
        if not is_text_string(section):
            raise RuntimeError("section is not a text string")
        if not is_text_string(option):
            raise RuntimeError("option is not a text string")

    def _get_default_or_raise(self, section, option):
        """
        Returns the value listed in the defaults if it exists
        :param section: A string denoting the section (not checked)
        :param option: A string denoting the option name
        :return: The value of the default
        :raises KeyError: if the item does not exist
        """
        value = None
        if self.defaults and section in self.defaults:
            try:
                value = self.defaults[section][option]
            except KeyError:
                raise KeyError("Unknown config item requested: " +
                               self._settings_path(section, option))
        return value

    def _settings_path(self, section, option):
        """
        Private method to construct a path to the given option with the
        section
        :param section: The name of the section
        :param option: The name of the option
        :return: A path to the location within the QSettings instance
        """
        self._check_section_option_is_valid(section, option)
        return section + "/" + option
