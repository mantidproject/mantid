# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from qtpy.QtCore import QSettings
from typing import Any, Type

from mantidqt.utils.qt.qsettings_change_aware import QSettingsChangeAware

# Settings in this set are stored per experiment, under an "RB_SCOPE/<number>/" subgroup, so different
# experiments can carry different values. Everything not in SCOPED SETTINGS remains global
RB_SCOPE = "rb/"
SCOPED_SETTINGS = {
    "rd_name",
    "nd_name",
    "td_name",
    "rd_dir",
    "nd_dir",
    "td_dir",
    "monte_carlo_params",
    "clear_absorption_ws_after_processing",
    "cost_func_thresh",
    "peak_pos_thresh",
    "use_euler_angles",
    "euler_angles_scheme",
    "euler_angles_sense",
    "plot_exp_pf",
    "contour_kernel",
}


def _clean_rb(rb: str | None) -> str | None:
    rb = (rb or "").strip()
    if not rb:
        return None
    # RB numbers may contain "/" and "\" and would be interpreted as the same path but create
    # a save subgroup
    # use "$" to replace as it is not in the RB field's permitted character set
    return rb.replace("/", "$").replace("\\", "$")


def get_scoped_prefix(prefix: str, rb: str | None) -> str:
    rb = _clean_rb(rb)
    return prefix if not rb else f"{prefix}{RB_SCOPE}{rb}/"


def _write_prefix(prefix: str, rb: str | None, setting_name: str) -> str:
    if setting_name not in SCOPED_SETTINGS:
        return prefix
    return get_scoped_prefix(prefix, rb)


def _read_prefix(group: str, prefix: str, rb: str | None, setting_name: str) -> str:
    if setting_name not in SCOPED_SETTINGS:
        return prefix
    scoped = get_scoped_prefix(prefix, rb)
    # if there is no rb set - use global prefix
    if scoped == prefix:
        return prefix
    settings = QSettings()
    settings.beginGroup(group)
    try:
        # fall back to the global value until this RB has one of its own
        return scoped if settings.contains(scoped + setting_name) else prefix
    finally:
        settings.endGroup()


def set_setting(group: str, prefix: str, setting_name: str, value: Any, rb: str | None = None) -> None:
    """
    Change or add a setting in the mantid .ini file.
    :param group: Settings group to pull from.
    :param prefix: Acts like a subgroup.
    :param setting_name: The key to the setting.
    :param value: The value of the setting.
    :param rb: Active RB number. Settings listed in SCOPED_SETTINGS are written under it.
    """
    prefix = _write_prefix(prefix, rb, setting_name)
    settings = QSettings()
    settings.beginGroup(group)
    QSettingsChangeAware(settings).setValue(prefix + setting_name, value)
    settings.endGroup()


def get_setting(group: str, prefix: str, setting_name: str, return_type: Type = str, rb: str | None = None) -> Any:
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
    :param rb: Active RB number. Settings listed in SCOPED_SETTINGS are read from it when it holds
        a value of its own, and inherit the global value otherwise.
    :return: The chosen setting.
    """
    prefix = _read_prefix(group, prefix, rb, setting_name)
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
