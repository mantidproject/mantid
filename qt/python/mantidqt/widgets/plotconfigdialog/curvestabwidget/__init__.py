# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import colors
from qtpy import QtCore


def convert_color_to_hex(color):
    """Convert a matplotlib color to its hex form"""
    try:
        return colors.cnames[color]
    except KeyError:
        return colors.to_hex(color)


class CurveProperties:

    def __init__(self):
        pass

    @classmethod
    def from_view(cls, view):
        # Top level entries
        cls.label = view.get_curve_label()
        cls.hide_curve = (view.get_hide_curve() == QtCore.Qt.Checked)

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

        # # Errorbars tab
        # cls.show_errorbars = view.get_show_errorbars()
        # cls.errorbar_width = view.get_errorbar_width()
        # cls.errorbar_capsize = view.get_errorbar_capsize()
        # cls.errorbar_cap_thickness = view.get_errorbar_cap_thickness()
        # cls.errobar_error_every = view.get_errobar_error_every()
        # cls.errobar_color = view.get_errobar_color()
        return cls()

    @classmethod
    def from_curve(cls, line):
        # Top level entries
        cls.label = line.get_label()
        cls.hide_curve = (not line.get_visible())

        # Line tab entries
        cls.line_style = line.get_linestyle()
        cls.draw_style = line.get_drawstyle()
        cls.line_width = line.get_linewidth()
        cls.line_color = convert_color_to_hex(line.get_color())

        # Marker tab entries
        cls.marker_style = line.get_marker()
        cls.marker_size = line.get_markersize()
        cls.marker_face_color = line.get_markerfacecolor()
        cls.marker_edge_color = line.get_markeredgecolor()

        # # Errorbars tab
        # cls.show_errorbars = line.get_show_errorbars()
        # cls.errorbar_width = line.get_errorbar_width()
        # cls.errorbar_capsize = line.get_errorbar_capsize()
        # cls.errorbar_cap_thickness = line.get_errorbar_cap_thickness()
        # cls.errobar_error_every = line.get_errobar_error_every()
        # cls.errobar_color = line.get_errobar_color()
        return cls()
