# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import colors
from matplotlib.axes import ErrorbarContainer
from qtpy.QtCore import Qt

MARKER_MAP = {'square': 's', 'plus (filled)': 'P', 'point': '.', 'tickdown': 3,
              'triangle_right': '>', 'tickup': 2, 'hline': '_', 'vline': '|',
              'pentagon': 'p', 'tri_left': '3', 'caretdown': 7,
              'caretright (centered at base)': 9, 'tickright': '1',
              'caretright': 5, 'caretleft': 4, 'tickleft': 0, 'tri_up': '2',
              'circle': 'o', 'pixel': ',', 'caretleft (centered at base)': 8,
              'diamond': 'D', 'star': '*', 'hexagon1': 'h', 'octagon': '8',
              'hexagon2': 'H', 'tri_right': '4', 'x (filled)': 'X',
              'thin_diamond': 'd', 'tri_down': 1, 'triangle_left': '<',
              'plus': '+', 'triangle_down': 'v', 'triangle_up': '^', 'x': 'x',
              'caretup': 6, 'caretup (centered at base)': 10,
              'caretdown (centered at base)': 11, 'None': 'None'}


def convert_color_to_hex(color):
    """Convert a matplotlib color to its hex form"""
    try:
        return colors.cnames[color]
    except (KeyError, TypeError):
        return colors.to_hex(color)


class CurveProperties:

    def __init__(self):
        pass

    @classmethod
    def from_view(cls, view, errorbars=None):
        # Top level entries
        cls.label = view.get_curve_label()
        cls.hide_curve = (view.get_hide_curve() == Qt.Checked)

        # Line tab entries
        cls.line_style = view.line.get_style()
        cls.draw_style = view.line.get_draw_style()
        cls.line_width = view.line.get_width()
        cls.line_color = view.line.get_color()

        # Marker tab entries
        cls.marker_style = view.marker.get_style()
        cls.marker_size = view.marker.get_size()
        cls.marker_face_color = view.marker.get_face_color()
        cls.marker_edge_color = view.marker.get_edge_color()

        # Errorbars tab
        if view.errorbars:
            cls.hide_errorbars = (view.errorbars.get_hide() == Qt.Checked)
            cls.errorbar_width = view.errorbars.get_width()
            cls.errorbar_capsize = view.errorbars.get_capsize()
            cls.errorbar_cap_thickness = view.errorbars.get_cap_thickness()
            cls.errorbar_error_every = view.errorbars.get_error_every()
            cls.errorbar_color = convert_color_to_hex(view.errorbars.get_color())
        return cls()

    @classmethod
    def from_curve(cls, curve):
        # Top level entries
        if isinstance(curve, ErrorbarContainer):
            line = curve.lines[0]
            caps_tuple = curve.lines[1]
            bars_tuple = curve.lines[2]

            # TODO: if cyclic values set for these, set to None and disable option
            cls.hide_errorbars = (not curve.lines[2][0].get_visible())
            cls.errorbar_width = bars_tuple[0].get_linewidth()[0]
            cls.errorbar_capsize = caps_tuple[0].get_markersize()/2
            cls.errorbar_cap_thickness = caps_tuple[0].get_markeredgewidth()
            # cls.errorbar_error_every = curve.get_error_every()
            cls.errorbar_error_every = 1
            cls.errorbar_color = convert_color_to_hex(bars_tuple[0].get_color()[0])
        else:
            line = curve

        cls.label = curve.get_label()
        cls.hide_curve = (not line.get_visible())

        # Line tab entries
        cls.line_style = line.get_linestyle()
        cls.draw_style = line.get_drawstyle()
        cls.line_width = line.get_linewidth()
        cls.line_color = convert_color_to_hex(line.get_color())

        # Marker tab entries
        cls.marker_style = line.get_marker()
        cls.marker_size = line.get_markersize()
        cls.marker_face_color = convert_color_to_hex(line.get_markerfacecolor())
        cls.marker_edge_color = convert_color_to_hex(line.get_markeredgecolor())

        return cls()
