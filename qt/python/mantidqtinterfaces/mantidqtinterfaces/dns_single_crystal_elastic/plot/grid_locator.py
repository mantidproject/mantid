# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import numpy as np
from mpl_toolkits.axisartist.grid_finder import (FixedLocator,
                                                 FormatterPrettyPrint,
                                                 MaxNLocator)
from mpl_toolkits.axisartist.grid_helper_curvelinear import \
    GridHelperCurveLinear


# functions to generate non orthogal grid in hkl
def get_grid_fromatter(gridstate):
    if gridstate == 0:
        locator = MaxNLocator(nbins='auto', steps=[1, 2, 3, 4, 5])
    else:
        locator = FixedLocator(np.arange(-30, 30, 1 / gridstate))
    tick_formatter = FormatterPrettyPrint()
    return locator, tick_formatter


def get_gridhelper_arguments(a, b, c, d, switch):
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


def get_gridhelper(gridhelper, gridstate, a, b, c, d, switch):
    # pylint: disable=too-many-arguments
    locator, tick_formatter = get_grid_fromatter(gridstate)
    inv_tr, tr = get_gridhelper_arguments(a, b, c, d, switch)
    if gridhelper is None:
        gridhelper = GridHelperCurveLinear((inv_tr, tr),
                                           grid_locator1=locator,
                                           grid_locator2=locator,
                                           tick_formatter1=tick_formatter,
                                           tick_formatter2=tick_formatter)
    else:
        gridhelper.update_grid_finder((inv_tr, tr),
                                      grid_locator1=locator,
                                      grid_locator2=locator,
                                      tick_formatter1=tick_formatter,
                                      tick_formatter2=tick_formatter)
    return gridhelper
