# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QSettings


def set_setting(group, prefix, setting_name, value):
    """
    Change or add a setting in the mantid .ini file.
    :param group: Settings group to pull from.
    :param prefix: Acts like a subgroup.
    :param setting_name: The key to the setting.
    :param value: The value of the setting.
    """
    settings = QSettings()
    settings.beginGroup(group)
    settings.setValue(prefix + setting_name, value)
    settings.endGroup()


def get_setting(group, prefix, setting_name, return_type=str):
    """
    Get a setting from the .ini file of mantid settings.

    NOTE: If you specify an int, but the setting contains a bool, you will get 0 for False
    and 1 for True, without a warning. Specifying bool will raise a TypeError if anything
    other than a bool or empty string is found in the settings. Not specifying a type will
    return a string. If nothing is found then an empty string is returned.

    :param group: Settings group to pull from.
    :param prefix: The prefix of the setting, acts like a subgroup.
    :param setting_name: Name of the setting.
    :param return_type: The type of the setting to get.
    :return: The chosen setting.
    """
    settings = QSettings()
    settings.beginGroup(group)
    if return_type is bool:
        setting = settings.value(prefix + setting_name, type=str)
        if setting == "":
            pass
        elif setting == "true":
            setting = True
        elif setting == "false":
            setting = False
        else:
            raise TypeError("Unable to convert string into valid bool")
    else:
        setting = settings.value(prefix + setting_name, type=return_type)
    settings.endGroup()
    return setting
