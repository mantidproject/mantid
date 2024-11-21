# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import rcParams
from matplotlib.container import ErrorbarContainer
from matplotlib.lines import Line2D
from numpy import isclose
from qtpy.QtCore import Qt

from mantid.plots.mantidaxes import MantidAxes
from mantid.plots.datafunctions import errorbars_hidden
from mantid.plots.utility import convert_color_to_hex

LINESTYLE_MAP = {"-": "solid", "--": "dashed", "-.": "dashdot", ":": "dotted", "None": "None"}

MARKER_MAP = {
    "square": "s",
    "plus (filled)": "P",
    "point": ".",
    "tickdown": 3,
    "triangle_right": ">",
    "tickup": 2,
    "hline": "_",
    "vline": "|",
    "pentagon": "p",
    "tri_left": "3",
    "caretdown": 7,
    "caretright (centered at base)": 9,
    "tickright": 1,
    "caretright": 5,
    "caretleft": 4,
    "tickleft": 0,
    "tri_up": "2",
    "circle": "o",
    "pixel": ",",
    "caretleft (centered at base)": 8,
    "diamond": "D",
    "star": "*",
    "hexagon1": "h",
    "octagon": "8",
    "hexagon2": "H",
    "tri_right": "4",
    "x (filled)": "X",
    "thin_diamond": "d",
    "tri_down": "1",
    "triangle_left": "<",
    "plus": "+",
    "triangle_down": "v",
    "triangle_up": "^",
    "x": "x",
    "caretup": 6,
    "caretup (centered at base)": 10,
    "caretdown (centered at base)": 11,
    "None": "None",
}


def get_ax_from_curve(curve):
    """Get the Axes a Line2D or ErrorbarContainer sits on"""
    if isinstance(curve, Line2D):
        return curve.axes
    elif isinstance(curve, ErrorbarContainer):
        return curve[2][0].axes


def get_marker_name(marker):
    """Get a marker's full name from its shorthand"""
    for name, short_name in MARKER_MAP.items():
        if short_name == marker:
            return name


def curve_hidden(curve):
    """Return True if curve is not visible"""
    if isinstance(curve, ErrorbarContainer):
        line_hidden = True if not curve[0] else (not curve[0].get_visible())
        bars_hidden = errorbars_hidden(curve)
        return line_hidden and bars_hidden
    else:
        return not curve.get_visible()


def remove_curve_from_ax(curve):
    """
    Remove a Line2D or ErrobarContainer from its Axes
    :param curve: A Line2D or ErrorbarContainer object
    """
    ax = get_ax_from_curve(curve)
    if isinstance(ax, MantidAxes):
        ax.remove_artists_if(lambda art: art == curve)
    else:
        curve.remove()
        if isinstance(curve, ErrorbarContainer):
            ax.containers.remove(curve)


def curve_has_errors(curve):
    """
    Return True if there are errors associated with a Line2D or
    ErrorbarContainer object. This checks whether there is a Workspace
    associated with the curve and checks if that workspace has errors and
    returns True if it does.
    """
    if isinstance(curve, ErrorbarContainer):
        return True
    ax = get_ax_from_curve(curve)
    if isinstance(ax, MantidAxes):
        try:
            return ax.artists_workspace_has_errors(curve)
        except ValueError:  # Value error raised if artist has no associated workspace
            return False
    if isinstance(curve, Line2D):
        return False


class CurveProperties(dict):
    def __init__(self, props):
        self.update(props)

    def __eq__(self, other):
        for prop, value in sorted(self.items()):
            other_value = other[prop]
            if isinstance(value, float):
                if not isclose(value, other_value, atol=1e-5):
                    return False
            else:
                if value != other_value:
                    return False
        return True

    def __getattr__(self, item):
        return self[item]

    def get_plot_kwargs(self):
        """Return curve properties that can be used as plot kwargs"""
        kwargs = {}
        for k, v in self.items():
            if k not in ["hide", "hide_errors"]:
                kwargs[k] = v
        kwargs["visible"] = not self.hide

        # If the long form of the marker name is currently being used, it is changed to the short form which is
        # understood by matplotlib.
        if kwargs["marker"] in MARKER_MAP:
            kwargs["marker"] = MARKER_MAP[kwargs["marker"]]

        return kwargs

    @classmethod
    def from_view(cls, view):
        """Get curve properties from the view"""
        props = dict()
        props["label"] = view.get_curve_label()
        props["hide"] = view.get_hide_curve() == Qt.Checked
        # Line props
        props["linestyle"] = view.line.get_style()
        props["drawstyle"] = view.line.get_draw_style()
        props["linewidth"] = view.line.get_width()
        props["color"] = view.line.get_color()
        # Marker props
        props["marker"] = MARKER_MAP[view.marker.get_style()]
        props["markersize"] = view.marker.get_size()
        props["markerfacecolor"] = view.marker.get_face_color()
        props["markeredgecolor"] = view.marker.get_edge_color()
        # Errorbar props
        if view.errorbars.isEnabled():
            props["hide_errors"] = view.errorbars.get_hide()
            props["errorevery"] = view.errorbars.get_error_every()
            props["capsize"] = view.errorbars.get_capsize()
            props["capthick"] = view.errorbars.get_cap_thickness()
            props["ecolor"] = view.errorbars.get_color()
            # setting errorbar line width to 0 sets width to default, so add a
            # little bit on to avoid this
            props["elinewidth"] = view.errorbars.get_width() + 1e-6
        return cls(props)

    @classmethod
    def from_curve(cls, curve):
        """Get curve properties from a Line2D or ErrorbarContainer"""
        props = dict()
        props["label"] = curve.get_label()
        props["hide"] = curve_hidden(curve)
        props = CurveProperties._get_errorbars_props_from_curve(curve, props)
        if isinstance(curve, ErrorbarContainer):
            curve = curve[0]
        props = CurveProperties._get_line_props_from_curve(curve, props)
        props = CurveProperties._get_marker_props_from_curve(curve, props)
        return cls(props)

    @staticmethod
    def _get_line_props_from_curve(curve, props):
        """Get a curve's line properties and add to props dict"""
        if not curve:
            props["linestyle"] = "None"
            props["drawstyle"] = "default"
            props["linewidth"] = rcParams["lines.linewidth"]
            props["color"] = convert_color_to_hex(rcParams["lines.color"])
        else:
            props["linestyle"] = LINESTYLE_MAP[curve.get_linestyle()]
            props["drawstyle"] = curve.get_drawstyle()
            props["linewidth"] = curve.get_linewidth()
            props["color"] = convert_color_to_hex(curve.get_color())
        return props

    @staticmethod
    def _get_marker_props_from_curve(curve, props):
        """Get a curve's marker properties and add to props dict"""
        if not curve:
            props["marker"] = "None"
            props["markersize"] = rcParams["lines.markersize"]
            props["markerfacecolor"] = convert_color_to_hex(rcParams["lines.color"])
            props["markeredgecolor"] = convert_color_to_hex(rcParams["lines.color"])
        else:
            props["marker"] = get_marker_name(curve.get_marker())
            props["markersize"] = curve.get_markersize()
            props["markerfacecolor"] = convert_color_to_hex(curve.get_markerfacecolor())
            props["markeredgecolor"] = convert_color_to_hex(curve.get_markeredgecolor())
        return props

    @staticmethod
    def _get_errorbars_props_from_curve(curve, props):
        """Get a curve's errorbar properties and add to props dict"""
        props["hide_errors"] = getattr(curve, "hide_errors", errorbars_hidden(curve))
        # ErrorbarContainer does not have 'errorevery' as an attribute directly
        # So to get this property take from errorbar lines curve
        try:
            barlines = curve[2][0]
            props["errorevery"] = int(barlines.axes.creation_args[len(barlines.axes.creation_args) - 1]["errorevery"])
        except (IndexError, TypeError, KeyError, AttributeError):
            props["errorevery"] = 1
        try:
            caps = curve[1]
            props["capsize"] = float(caps[0].get_markersize() / 2)
            props["capthick"] = float(caps[0].get_markeredgewidth())
        except (IndexError, TypeError):
            props["capsize"] = 0.0
            props["capthick"] = 1.0
        try:
            bars = curve[2]
            props["elinewidth"] = float(bars[0].get_linewidth()[0])
            props["ecolor"] = convert_color_to_hex(bars[0].get_color()[0])
        except (IndexError, TypeError):
            props["elinewidth"] = 1.0
            # if the errorbars don't have a default color, use the line's color
            props["ecolor"] = curve.get_color()
        return props
