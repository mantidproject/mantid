# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from qtpy.QtCore import QSettings


def set_setting(group, prefix, setting_name, value):
    settings = QSettings()
    settings.beginGroup(group)
    settings.setValue(prefix + setting_name, value)
    settings.endGroup()


def get_setting(group, prefix, setting_name):
    settings = QSettings()
    settings.beginGroup(group)
    setting = settings.value(prefix + setting_name)
    settings.endGroup()
    return setting
