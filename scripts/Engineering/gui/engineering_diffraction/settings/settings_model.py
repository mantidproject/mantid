# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting, set_setting
from Engineering.gui.engineering_diffraction.tabs.common import output_settings


class SettingsModel(object):
    def get_settings_dict(self, names_and_types):
        settings = {}
        for setting_name in names_and_types.keys():
            settings[setting_name] = self.get_setting(setting_name, return_type=names_and_types[setting_name])
        return settings

    def set_settings_dict(self, settings):
        for key in settings:
            self.set_setting(key, settings[key])

    @staticmethod
    def get_setting(name, return_type=str):
        return get_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name,
                           return_type=return_type)

    @staticmethod
    def set_setting(name, value):
        set_setting(output_settings.INTERFACES_SETTINGS_GROUP, output_settings.ENGINEERING_PREFIX, name, value)
