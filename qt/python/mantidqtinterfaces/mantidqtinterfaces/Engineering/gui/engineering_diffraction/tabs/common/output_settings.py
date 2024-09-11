# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Holds some common constants across all tabs.
"""

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting

INTERFACES_SETTINGS_GROUP = "CustomInterfaces"
ENGINEERING_PREFIX = "EngineeringDiffraction2/"


def get_output_path():
    location = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "save_location")
    return location if location is not None else ""


def get_path_to_gsas2():
    location = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "path_to_gsas2")
    return location if location is not None else ""
