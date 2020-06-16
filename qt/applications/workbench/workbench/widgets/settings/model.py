# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench
import ast

from mantid import ConfigService
from workbench.config import CONF
from workbench.widgets.settings.general.presenter import GeneralProperties


class SettingsModel:

    @staticmethod
    def save_settings_to_file(filepath, settings):
        with open(filepath, 'w') as file:
            for setting in settings:
                if CONF.has(setting):
                    if setting != GeneralProperties.USER_LAYOUT.value:
                        value = CONF.get(setting)
                else:
                    value = ConfigService.getString(setting)
                file.write(setting + '=' + str(value) + '\n')

    @staticmethod
    def load_settings_from_file(filepath, settings):
        with open(filepath, 'r') as file:
            for line in file:
                setting = line.rstrip('\n').split('=', 1)
                prop = setting[0].strip()
                if prop in settings:
                    if CONF.has(prop) or prop.find('/') != -1:
                        if prop == GeneralProperties.USER_LAYOUT.value:
                            pass
                        elif prop == GeneralProperties.FONT.value and not setting[1]:
                            # if the font setting is empty removing it will make workbench use the default
                            CONF.remove(prop)
                        else:
                            try:
                                value = ast.literal_eval(setting[1])
                                pass
                            except (SyntaxError, ValueError):
                                value = setting[1]
                            CONF.set(setting[0].strip(), value)
                    else:
                        ConfigService.setString(prop, setting[1])
