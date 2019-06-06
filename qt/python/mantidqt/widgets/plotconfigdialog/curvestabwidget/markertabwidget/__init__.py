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

MARKER_MAP = {'square': 's', 'plus (filled)': 'P', 'point': '.', 'tickdown': 3,
              'triangle_right': '>', 'tickup': 2, 'hline': '_', 'vline': '|',
              'pentagon': 'p', 'tri_left': '3', 'caretdown': 7,
              'caretright (centered at base)': 9, 'tickright': '1',
              'caretright': 5, 'caretleft': 4, 'tickleft': 0, 'tri_up': '2',
              'circle': 'o', 'pixel': ',', 'caretleft (centered at base)': 8,
              'diamond': 'D', 'star': '*', 'hexagon1': 'h', 'octagon': '8',
              'hexagon2': 'H', 'tri_right': '4', 'x (filled)': 'X',
              'thin_diamond': 'd', 'tri_down': '1', 'triangle_left': '<',
              'plus': '+', 'triangle_down': 'v', 'triangle_up': '^', 'x': 'x',
              'caretup': 6, 'caretup (centered at base)': 10,
              'caretdown (centered at base)': 11, 'None': 'None'}


def get_marker_name(marker):
    for name, short_name in MARKER_MAP.items():
        if short_name == marker:
            return name


class MarkerProperties:

    def __init__(self, props):
        for prop, value in props.items():
            setattr(self, prop, value)

    @classmethod
    def from_view(cls, view):
        if not view.isEnabled():
            return None
        props = dict()
        props['style'] = view.get_style()
        props['size'] = view.get_size()
        props['face_color'] = view.get_face_color()
        props['edge_color'] = view.get_edge_color()
        return cls(props)

    @classmethod
    def from_line(cls, line):
        if not isinstance(line, Line2D):
            return None
        props = dict()
        props['style'] = get_marker_name(line.get_marker())
        props['size'] = line.get_markersize()
        props['face_color'] = convert_color_to_hex(line.get_markerfacecolor())
        props['edge_color'] = convert_color_to_hex(line.get_markeredgecolor())
        return cls(props)
