# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from __future__ import (absolute_import, unicode_literals)

from matplotlib.lines import Line2D

from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex

LINESTYLE_MAP = {'-': 'solid', '--': 'dashed', '-.': 'dashdot', ':': 'dotted',
                 'None': 'None'}


class LineProperties:

    def __init__(self, props):
        for prop, value in props.items():
            setattr(self, prop, value)

    @classmethod
    def from_view(cls, view):
        if not view.line.isEnabled():
            return None
        props = dict()
        props['style'] = view.line.get_style()
        props['draw_style'] = view.line.get_draw_style()
        props['width'] = view.line.get_width()
        props['color'] = view.line.get_color()
        return cls(props)

    @classmethod
    def from_line(cls, line):
        if not isinstance(line, Line2D):
            return None
        props = dict()
        props['style'] = LINESTYLE_MAP[line.get_linestyle()]
        props['draw_style'] = line.get_drawstyle()
        props['width'] = line.get_linewidth()
        props['color'] = convert_color_to_hex(line.get_color())
        return cls(props)
