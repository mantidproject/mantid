# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from Muon.GUI.Common.ADSHandler.ADS_calls import check_if_workspace_exist, retrieve_ws

DEFAULT_X_LOWER = 0.0
DEFAULT_X_UPPER = 100.0
X_OFFSET = 0.001


def x_limits_of_workspace(workspace_name: str, default_limits: tuple = (DEFAULT_X_LOWER, DEFAULT_X_UPPER)) -> tuple:
    """Returns the x data limits of a provided workspace."""
    if workspace_name is not None and check_if_workspace_exist(workspace_name):
        x_data = retrieve_ws(workspace_name).dataX(0)
        if len(x_data) > 0:
            x_data.sort()
            x_lower, x_higher = x_data[0], x_data[-1]
            if x_lower == x_higher:
                return x_lower - X_OFFSET, x_higher + X_OFFSET
            return x_lower, x_higher
    return default_limits


def is_equal_to_n_decimals(value1: float, value2: float, n_decimals: int) -> bool:
    """Checks that two floats are equal up to n decimal places."""
    return f"{value1:.{n_decimals}f}" == f"{value2:.{n_decimals}f}"


def _check_start_x_is_valid(x_lower: float, x_upper: float, view_start_x: float, view_end_x: float,
                            model_start_x: float, check_is_equal: bool = True) -> tuple:
    """Check that the start x is valid. If it isn't, the exclude range is adjusted to be valid."""
    if view_start_x < x_lower:
        new_start_x, new_end_x = x_lower, view_end_x
    elif view_start_x > x_upper:
        if not is_equal_to_n_decimals(view_end_x, x_upper, 3):
            new_start_x, new_end_x = view_end_x, x_upper
        else:
            new_start_x, new_end_x = model_start_x, view_end_x
    elif view_start_x > view_end_x:
        new_start_x, new_end_x = view_end_x, view_start_x
    elif check_is_equal and view_start_x == view_end_x:
        new_start_x, new_end_x = model_start_x, view_end_x
    else:
        new_start_x, new_end_x = view_start_x, view_end_x

    return new_start_x, new_end_x


def _check_end_x_is_valid(x_lower: float, x_upper: float, view_start_x: float, view_end_x: float, model_end_x: float,
                          check_is_equal: bool = True) -> tuple:
    """Check that the exclude end x is valid. If it isn't, the exclude range is adjusted to be valid."""
    if view_end_x < x_lower:
        if not is_equal_to_n_decimals(view_start_x, x_lower, 3):
            new_start_x, new_end_x = x_lower, view_start_x
        else:
            new_start_x, new_end_x = view_start_x, model_end_x
    elif view_end_x > x_upper:
        new_start_x, new_end_x = view_start_x, x_upper
    elif view_end_x < view_start_x:
        new_start_x, new_end_x = view_end_x, view_start_x
    elif check_is_equal and view_end_x == view_start_x:
        new_start_x, new_end_x = view_start_x, model_end_x
    else:
        new_start_x, new_end_x = view_start_x, view_end_x

    return new_start_x, new_end_x


def check_start_x_is_valid(workspace_name: str, view_start_x: float, view_end_x: float, model_start_x: float) -> tuple:
    """Checks that the new start X is valid. If it isn't, the start and end X is adjusted."""
    x_lower, x_upper = x_limits_of_workspace(workspace_name)
    return _check_start_x_is_valid(x_lower, x_upper, view_start_x, view_end_x, model_start_x)


def check_end_x_is_valid(workspace_name: str, view_start_x: float, view_end_x: float, model_end_x: float) -> tuple:
    """Checks that the new end X is valid. If it isn't, the start and end X is adjusted."""
    x_lower, x_upper = x_limits_of_workspace(workspace_name)
    return _check_end_x_is_valid(x_lower, x_upper, view_start_x, view_end_x, model_end_x)


def check_exclude_start_x_is_valid(x_lower: float, x_upper: float, view_exclude_start_x: float,
                                   view_exclude_end_x: float, model_exclude_start_x: float) -> tuple:
    """Check that the exclude start x is valid. If it isn't, the exclude range is adjusted to be valid."""
    return _check_start_x_is_valid(x_lower, x_upper, view_exclude_start_x, view_exclude_end_x, model_exclude_start_x,
                                   check_is_equal=False)


def check_exclude_end_x_is_valid(x_lower: float, x_upper: float, view_exclude_start_x: float, view_exclude_end_x: float,
                                 model_exclude_end_x: float) -> tuple:
    """Check that the exclude end x is valid. If it isn't, the exclude range is adjusted to be valid."""
    return _check_end_x_is_valid(x_lower, x_upper, view_exclude_start_x, view_exclude_end_x, model_exclude_end_x,
                                 check_is_equal=False)


def check_exclude_start_and_end_x_is_valid(x_lower: float, x_upper: float, exclude_start_x: float,
                                           exclude_end_x: float) -> tuple:
    """Check that the new exclude start and end X are valid. If not they are adjusted."""
    if exclude_start_x < x_lower or exclude_start_x > x_upper:
        exclude_start_x = x_upper
    if exclude_end_x < x_lower or exclude_end_x > x_upper:
        exclude_end_x = x_upper

    if exclude_start_x > exclude_end_x:
        exclude_start_x, exclude_end_x = exclude_end_x, exclude_start_x
    return exclude_start_x, exclude_end_x
