# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from __future__ import (absolute_import, division, print_function)

from os import path
from Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

INTERFACES_SETTINGS_GROUP = "CustomInterfaces"
ENGINEERING_PREFIX = "EngineeringDiffraction2/"


def get_run_number_from_path(run_path, instrument):
    return path.splitext(path.basename(run_path))[0].replace(instrument, '').lstrip('0')


def get_output_path():
    location = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "save_location")
    return location if location is not None else ""
