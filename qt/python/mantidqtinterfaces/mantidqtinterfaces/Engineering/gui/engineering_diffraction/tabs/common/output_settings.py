# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
"""
Holds some common constants across all tabs.
"""

from mantidqtinterfaces.Engineering.gui.engineering_diffraction.settings.settings_helper import get_setting
from numpy import array, concatenate, ndarray
from typing import Tuple

INTERFACES_SETTINGS_GROUP = "CustomInterfaces"
ENGINEERING_PREFIX = "EngineeringDiffraction2/"


def get_output_path() -> str:
    location = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "save_location")
    return str(location) if location is not None else ""


def get_path_to_gsas2() -> str:
    location = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "path_to_gsas2")
    return str(location) if location is not None else ""


def get_timeout() -> int:
    timeout = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "timeout")
    return int(timeout) if timeout else 100


def get_texture_axes_transform(rb_num: str | None = None) -> Tuple[ndarray, Tuple[str, str, str]]:
    # the sample directions are RB-scoped, so every caller has to say which experiment is active,
    # otherwise directions saved against an RB are written but never read back
    rd_str = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "rd_dir", rb=rb_num)
    nd_str = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "nd_dir", rb=rb_num)
    td_str = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "td_dir", rb=rb_num)
    rd = array([float(x) for x in rd_str.split(",")])[:, None]
    nd = array([float(x) for x in nd_str.split(",")])[:, None]
    td = array([float(x) for x in td_str.split(",")])[:, None]
    rd_name = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "rd_name", rb=rb_num)
    nd_name = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "nd_name", rb=rb_num)
    td_name = get_setting(INTERFACES_SETTINGS_GROUP, ENGINEERING_PREFIX, "td_name", rb=rb_num)
    return concatenate([rd, nd, td], axis=1), (rd_name, nd_name, td_name)
