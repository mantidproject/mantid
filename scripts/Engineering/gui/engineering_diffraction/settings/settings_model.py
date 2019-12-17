# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from Engineering.gui.engineering_diffraction.tabs.common import path_handling


class SettingsModel(object):
    def get_settings_dict(self, keys):
        settings = {}
        for setting_name in keys:
            settings[setting_name] = self.get_setting(setting_name)
        return settings

    def set_settings_dict(self, settings):
        for key in settings:
            self.set_setting(key, settings[key])

    @staticmethod
    def get_setting(name):
        return get_setting(path_handling.INTERFACES_SETTINGS_GROUP, path_handling.ENGINEERING_PREFIX, name)

    @staticmethod
    def set_setting(name, value):
        set_setting(path_handling.INTERFACES_SETTINGS_GROUP, path_handling.ENGINEERING_PREFIX, name, value)
