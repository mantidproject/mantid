# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import rcParams
from matplotlib.axes import ErrorbarContainer
from matplotlib.lines import Line2D
from qtpy.QtCore import Qt

from mantid.plots import MantidAxes
from mantidqt.widgets.plotconfigdialog.colorselector import convert_color_to_hex

LINESTYLE_MAP = {'-': 'solid', '--': 'dashed', '-.': 'dashdot', ':': 'dotted',
                 'None': 'None'}

MARKER_MAP = {'square': 's', 'plus (filled)': 'P', 'point': '.', 'tickdown': 3,
              'triangle_right': '>', 'tickup': 2, 'hline': '_', 'vline': '|',
              'pentagon': 'p', 'tri_left': '3', 'caretdown': 7,
              'caretright (centered at base)': 9, 'tickright': 1,
              'caretright': 5, 'caretleft': 4, 'tickleft': 0, 'tri_up': '2',
              'circle': 'o', 'pixel': ',', 'caretleft (centered at base)': 8,
              'diamond': 'D', 'star': '*', 'hexagon1': 'h', 'octagon': '8',
              'hexagon2': 'H', 'tri_right': '4', 'x (filled)': 'X',
              'thin_diamond': 'd', 'tri_down': '1', 'triangle_left': '<',
              'plus': '+', 'triangle_down': 'v', 'triangle_up': '^', 'x': 'x',
              'caretup': 6, 'caretup (centered at base)': 10,
              'caretdown (centered at base)': 11, 'None': 'None'}


def errorbars_hidden(err_container):
    hidden = True
    try:
        for lines in err_container.lines:
            try:
                for line in lines:
                    hidden = hidden and (not line.get_visible())
            except TypeError:
                if lines:
                    hidden = hidden and (not lines.get_visible())
    except AttributeError:
        pass
    return hidden


def get_marker_name(marker):
    for name, short_name in MARKER_MAP.items():
        if short_name == marker:
            return name


def curve_hidden(curve):
    if isinstance(curve, ErrorbarContainer):
        return errorbars_hidden(curve)
    else:
        return not curve.get_visible()


def set_curve_hidden(curve, hide):
    if isinstance(curve, ErrorbarContainer):
        if curve[0]:
            curve[0].set_visible(not hide)
        set_errorbars_hidden(curve, hide)
    else:
        curve.set_visible(not hide)


def set_errorbars_hidden(container, hide):
    if not isinstance(container, ErrorbarContainer):
        return
    if container[1]:
        for caps in container[1]:
            caps.set_visible(not hide)
    if container[2]:
        for bars in container[2]:
            bars.set_visible(not hide)


def get_ax_from_curve(curve):
    if isinstance(curve, Line2D):
        return curve.axes
    elif isinstance(curve, ErrorbarContainer):
        return curve[2][0].axes


def curve_has_errors(curve):
    ax = get_ax_from_curve(curve)
    if isinstance(curve, Line2D):
        return False
    if isinstance(curve, ErrorbarContainer):
        return True
    if isinstance(ax, MantidAxes):
        try:
            workspace, spec_num = ax.get_artists_workspace_and_spec_num(curve)
            workspace_index = workspace.getIndexFromSpectrumNumber(spec_num)
            if any(workspace.readE(workspace_index) != 0):
                return True
            else:
                return False
        except ValueError:
            return False


class CurveProperties:

    def __init__(self, props):
        self.__dict__.update(props)

    def to_dict(self):
        return self.__dict__

    def get_plot_kwargs(self):
        kwargs = {}
        for k, v in self.to_dict().items():
            if k not in ['hide', 'hide_errors']:
                kwargs[k] = v
        return kwargs

    @classmethod
    def from_view(cls, view):
        props = dict()
        props['label'] = view.get_curve_label()
        props['hide'] = (view.get_hide_curve() == Qt.Checked)
        props = CurveProperties._get_line_props_from_view(view, props)
        props = CurveProperties._get_marker_props_from_view(view, props)
        props = CurveProperties._get_errorbar_props_from_view(view, props)
        return cls(props)

    @staticmethod
    def _get_line_props_from_view(view, props):
        props['linestyle'] = view.line.get_style()
        props['drawstyle'] = view.line.get_draw_style()
        props['linewidth'] = view.line.get_width()
        props['color'] = view.line.get_color()
        return props

    @staticmethod
    def _get_marker_props_from_view(view, props):
        props['marker'] = MARKER_MAP[view.marker.get_style()]
        props['markersize'] = view.marker.get_size()
        props['markerfacecolor'] = view.marker.get_face_color()
        props['markeredgecolor'] = view.marker.get_edge_color()
        return props

    @staticmethod
    def _get_errorbar_props_from_view(view, props):
        props['hide_errors'] = view.errorbars.get_hide()
        props['errorevery'] = view.errorbars.get_error_every()
        props['capsize'] = view.errorbars.get_capsize()
        props['capthick'] = view.errorbars.get_cap_thickness()
        props['ecolor'] = view.errorbars.get_color()
        # setting errorbar line width to 0 sets width to default, so add a
        # little bit on to avoid this
        props['elinewidth'] = view.errorbars.get_width() + 0.000001
        return props

    @classmethod
    def from_curve(cls, curve):
        props = dict()
        props['label'] = curve.get_label()
        props['hide'] = curve_hidden(curve)
        props = CurveProperties._get_errorbars_props_from_curve(curve, props)
        if isinstance(curve, ErrorbarContainer):
            curve = curve[0]
        props = CurveProperties._get_line_props_from_curve(curve, props)
        props = CurveProperties._get_marker_props_from_curve(curve, props)
        return cls(props)

    @staticmethod
    def _get_line_props_from_curve(curve, props):
        if not curve:
            props['linestyle'] = 'None'
            props['drawstyle'] = 'default'
            props['linewidth'] = rcParams['lines.linewidth']
            props['color'] = convert_color_to_hex(rcParams['lines.color'])
        else:
            props['linestyle'] = LINESTYLE_MAP[curve.get_linestyle()]
            props['drawstyle'] = curve.get_drawstyle()
            props['linewidth'] = curve.get_linewidth()
            props['color'] = convert_color_to_hex(curve.get_color())
        return props

    @staticmethod
    def _get_marker_props_from_curve(curve, props):
        if not curve:
            props['marker'] = 'None'
            props['markersize'] = rcParams['lines.markersize']
            props['markerfacecolor'] = convert_color_to_hex(rcParams['lines.color'])
            props['markeredgecolor'] = convert_color_to_hex(rcParams['lines.color'])
        else:
            props['marker'] = get_marker_name(curve.get_marker())
            props['markersize'] = curve.get_markersize()
            props['markerfacecolor'] = convert_color_to_hex(curve.get_markerfacecolor())
            props['markeredgecolor'] = convert_color_to_hex(curve.get_markeredgecolor())
        return props

    @staticmethod
    def _get_errorbars_props_from_curve(curve, props):
        props['hide_errors'] = errorbars_hidden(curve)
        props['errorevery'] = getattr(curve, 'errorevery', 1)
        try:
            caps = curve[1]
            props['capsize'] = caps[0].get_markersize()/2
            props['capthick'] = caps[0].get_markeredgewidth()
        except (IndexError, TypeError):
            props['capsize'] = 0
            props['capthick'] = 1
        try:
            bars = curve[2]
            props['elinewidth'] = bars[0].get_linewidth()[0]
            props['ecolor'] = convert_color_to_hex(bars[0].get_color()[0])
        except (IndexError, TypeError):
            props['elinewidth'] = 1
            props['ecolor'] = convert_color_to_hex(rcParams['lines.color'])
        return props
