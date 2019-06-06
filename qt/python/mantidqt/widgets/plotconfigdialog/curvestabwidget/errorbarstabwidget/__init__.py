# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.axes import ErrorbarContainer

from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex


def errorbars_hidden(err_container):
    hidden = True
    for lines in err_container.lines:
        try:
            for line in lines:
                hidden = hidden and (not line.get_visible())
        except TypeError:
            if lines:
                hidden = hidden and (not lines.get_visible())
    return hidden


class ErrorbarsProperties:

    def __init__(self, props):
        for prop, value in props.items():
            setattr(self, prop, value)

    @classmethod
    def from_view(cls, view):
        if not view.isEnabled():
            return None
        props = dict()
        props['hide'] = view.get_hide()
        props['width'] = view.get_width()
        props['capsize'] = view.get_capsize()
        props['cap_thickness'] = view.get_cap_thickness()
        props['error_every'] = view.get_error_every()
        props['color'] = view.get_color()
        return cls(props)

    @classmethod
    def from_container(cls, err_container):
        if not isinstance(err_container, ErrorbarContainer):
            return None

        caps_tuple = err_container.lines[1]
        bars_tuple = err_container.lines[2]
        props = dict()
        props['hide'] = errorbars_hidden(err_container)
        props['error_every'] = 1
        if caps_tuple:
            props['capsize'] = caps_tuple[0].get_markersize()/2
            props['cap_thickness'] = caps_tuple[0].get_markeredgewidth()
            props['color'] = convert_color_to_hex(caps_tuple[0].get_color())
            props['caps'] = True
        else:
            props['caps'] = None
        if bars_tuple:
            props['width'] = bars_tuple[0].get_linewidth()[0]
            # Bar color overrides cap color
            props['color'] = convert_color_to_hex(bars_tuple[0].get_color()[0])
            props['bars'] = True
        else:
            props['bars'] = None
        return cls(props)
