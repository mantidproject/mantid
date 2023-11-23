# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
Functions to generate non-orthogonal grid in hkl.
"""

import numpy as np
from mpl_toolkits.axisartist.grid_finder import FixedLocator, FormatterPrettyPrint, MaxNLocator
from mpl_toolkits.axisartist.grid_helper_curvelinear import GridHelperCurveLinear


def get_grid_formatter(grid_state):
    if grid_state == 0:
        locator = MaxNLocator(nbins="auto", steps=[1, 2, 3, 4, 5])
    else:
        locator = FixedLocator(np.arange(-30, 30, 1 / grid_state))
    tick_formatter = FormatterPrettyPrint()
    return locator, tick_formatter


def get_grid_helper_arguments(a, b, c, d, switch):
    def tr(x, y):
        if switch:
            x, y = y, x
        x, y = np.asarray(x), np.asarray(y)
        h = a * x + b * y
        k = c * x + d * y
        if switch:
            h, k = k, h
        return h, k

    def inv_tr(h, k):
        if switch:
            h, k = k, h
        h, k = np.asarray(h), np.asarray(k)
        x = (b * k - d * h) / (b * c - a * d)
        y = (c * h - a * k) / (b * c - a * d)
        if switch:
            x, y = y, x
        return x, y

    return [inv_tr, tr]


def get_grid_helper(grid_helper, grid_state, a, b, c, d, switch):
    # pylint: disable=too-many-arguments
    locator, tick_formatter = get_grid_formatter(grid_state)
    inv_tr, tr = get_grid_helper_arguments(a, b, c, d, switch)
    if grid_helper is None:
        grid_helper = GridHelperCurveLinear(
            (inv_tr, tr), grid_locator1=locator, grid_locator2=locator, tick_formatter1=tick_formatter, tick_formatter2=tick_formatter
        )
    else:
        grid_helper.update_grid_finder(
            (inv_tr, tr), grid_locator1=locator, grid_locator2=locator, tick_formatter1=tick_formatter, tick_formatter2=tick_formatter
        )
    return grid_helper
