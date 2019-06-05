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

LINESTYLE_MAP = {'-': 'solid', '--': 'dashed', '-.': 'dashdot', ':': 'dotted',
                 'None': 'None'}
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


def get_marker_name(marker):
    for name, short_name in MARKER_MAP.items():
        if short_name == marker:
            return name


class CurveProperties:

    def __init__(self, props_dict):
        for prop, value in props_dict.items():
            setattr(self, prop, value)

    @classmethod
    def from_view(cls, view):
        props = dict()
        # Top level entries
        props['label'] = view.get_curve_label()
        props['hide_curve'] = (view.get_hide_curve() == Qt.Checked)

        # Line tab entries
        props['line_style'] = view.line.get_style()
        props['draw_style'] = view.line.get_draw_style()
        props['line_width'] = view.line.get_width()
        props['line_color'] = view.line.get_color()

        # Marker tab entries
        props['marker_style'] = view.marker.get_style()
        props['marker_size'] = view.marker.get_size()
        props['marker_face_color'] = view.marker.get_face_color()
        props['marker_edge_color'] = view.marker.get_edge_color()

        # Errorbars tab
        if view.errorbars.isEnabled():
            props['hide_errorbars'] = (view.errorbars.get_hide() == Qt.Checked)
            props['errorbar_width'] = view.errorbars.get_width()
            props['errorbar_capsize'] = view.errorbars.get_capsize()
            props['errorbar_cap_thickness'] = view.errorbars.get_cap_thickness()
            props['errorbar_error_every'] = view.errorbars.get_error_every()
            props['errorbar_color'] = convert_color_to_hex(view.errorbars.get_color())
        return cls(props)

    @classmethod
    def from_curve(cls, curve):
        props = dict()
        props['label'] = curve.get_label()
        if isinstance(curve, ErrorbarContainer):
            line = curve.lines[0]
            caps_tuple = curve.lines[1]
            bars_tuple = curve.lines[2]

            props['hide_errorbars'] = True
            if caps_tuple:
                props['errorbar_capsize'] = caps_tuple[0].get_markersize()/2
                props['errorbar_cap_thickness'] = caps_tuple[0].get_markeredgewidth()
                props['errorbar_color'] = convert_color_to_hex(caps_tuple[0].get_color())
                props['hide_errorbars'] = (props['hide_errorbars'] and
                                           not caps_tuple[0].get_visible())
            if bars_tuple:
                props['errorbar_width'] = bars_tuple[0].get_linewidth()[0]
                props['errorbar_color'] = convert_color_to_hex(bars_tuple[0].get_color()[0])
                props['hide_errorbars'] = (props['hide_errorbars'] and
                                           not bars_tuple[0].get_visible())
            props['errorbar_error_every'] = 1
        else:
            line = curve

        if line:
            props['hide_curve'] = (not line.get_visible())

            # Line tab entries
            props['line_style'] = LINESTYLE_MAP[line.get_linestyle()]
            props['draw_style'] = line.get_drawstyle()
            props['line_width'] = line.get_linewidth()
            props['line_color'] = convert_color_to_hex(line.get_color())

            # Marker tab entries
            props['marker_style'] = get_marker_name(line.get_marker())
            props['marker_size'] = line.get_markersize()
            props['marker_face_color'] = convert_color_to_hex(line.get_markerfacecolor())
            props['marker_edge_color'] = convert_color_to_hex(line.get_markeredgecolor())

        return cls(props)
