# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.

from matplotlib import rcParams
from matplotlib.font_manager import FontProperties
from mantid.plots.legend import LegendProperties
from mantid.plots.utility import convert_color_to_hex
from workbench.plotting.plotscriptgenerator.utils import convert_args_to_string

# Default values of all options that are accessible via the legend tab in the plot settings.
mpl_default_kwargs = {
    'visible': True,
    'title': '',
    'background_color': convert_color_to_hex(rcParams['axes.facecolor']),  # inherits from axes by default
    'edge_color': convert_color_to_hex(rcParams['legend.edgecolor']),
    'transparency': rcParams['legend.framealpha'],
    'entries_font': 'DejaVu Sans',
    'entries_size': rcParams['legend.fontsize'],
    'entries_color': '#000000',
    'title_font': 'DejaVu Sans',
    'title_size': rcParams['axes.labelsize'],  # Uses axes size by default
    'title_color': '#000000',
    'marker_size': rcParams['legend.handlelength'],
    'box_visible': rcParams['legend.frameon'],
    'shadow': rcParams['legend.shadow'],
    'round_edges': rcParams['legend.fancybox'],
    'columns': 1,
    'column_spacing': rcParams['legend.columnspacing'],
    'label_spacing': rcParams['legend.labelspacing'],
    'marker_position': "Left of Entries",
    'markers': rcParams['legend.numpoints'],
    'border_padding': rcParams['legend.borderpad'],
    'marker_label_padding': rcParams['legend.handletextpad']
}

# Dictionary to convert from the mantid legend interface to matplotlib legend argument names.
MANTID_TO_MPL = {
    'background_color': 'facecolor',
    'edge_color': 'edgecolor',
    'transparency': 'framealpha',
    'entries_size': 'fontsize',
    'columns': 'ncol',
    'markers': 'numpoints',
    'marker_position': 'markerfirst',
    'box_visible': 'frameon',
    'round_edges': 'fancybox',
    'shadow': 'shadow',
    'title': 'title',
    'border_padding': 'borderpad',
    'label_spacing': 'labelspacing',
    'marker_size': 'handlelength',
    'marker_label_padding': 'handletextpad',
    'column_spacing': 'columnspacing'
}


def generate_legend_commands(legend):
    """
    Generates a string containing a comma separated list of kwargs to set legend properties.
    """
    kwargs = get_legend_command_kwargs(legend)
    return convert_args_to_string([], kwargs)


def generate_title_font_commands(legend, legend_object_var):
    """
    Generate commands for setting properties for the legend title font.
    """
    title_commands = []
    kwargs = LegendProperties.from_legend(legend)
    _remove_kwargs_if_default(kwargs)

    if 'title_font' in kwargs:
        title_commands.append(legend_object_var + ".get_title().set_fontname('" + kwargs['title_font'] + "')")
    if 'title_color' in kwargs:
        title_commands.append(legend_object_var + ".get_title().set_color('" + kwargs['title_color'] + "')")
    if 'title_size' in kwargs:
        title_commands.append(legend_object_var + ".get_title().set_fontsize('" + str(kwargs['title_size']) + "')")

    return title_commands


def generate_label_font_commands(legend, legend_object_var):
    """
    Generate python commands for setting the legend text label properties. The size is not present here because it is
    already included in the list of legend properties.
    """
    label_commands = []
    kwargs = LegendProperties.from_legend(legend)
    _remove_kwargs_if_default(kwargs)
    if 'entries_font' in kwargs:
        label_commands.append("[label.set_fontname('" + kwargs['entries_font']
                              + "') for label in " + legend_object_var + ".get_texts()]")
    if 'entries_color' in kwargs:
        label_commands.append("[label.set_color('" + kwargs['entries_color']
                              + "') for label in " + legend_object_var + ".get_texts()]")

    return label_commands


def generate_visible_command(legend, legend_object_var):
    """
    Returns a command to set the visibility of the legend if it's different to the default value.
    It's returned as a list for convenience, so it can be added to the end of a list without checking if it's empty.
    """
    visible_command = []
    kwargs = LegendProperties.from_legend(legend)
    _remove_kwargs_if_default(kwargs)
    if 'visible' in kwargs:
        visible_command.append(legend_object_var + ".set_visible(" + str(kwargs['visible']) + ")")
    return visible_command


def get_legend_command_kwargs(legend):
    """
    Returns a list of matplotlib legend kwargs, removing any that are default values.
    """
    kwargs = LegendProperties.from_legend(legend)
    _remove_kwargs_if_default(kwargs)
    # Convert the kwargs to the matplotlib ones.
    return get_mpl_kwargs(kwargs)


def get_mpl_kwargs(kwargs):
    """
    Keep only matplotlib legend kwargs, and convert the keys to matplotlib compatible ones.
    """
    mpl_kwargs = {}
    for key, value in kwargs.items():
        if key in MANTID_TO_MPL:
            mpl_kwargs[MANTID_TO_MPL[key]] = value

    # The markerfirst kwarg is a boolean in matplotlib, so need to convert it.
    if 'markerfirst' in mpl_kwargs:
        mpl_kwargs['markerfirst'] = mpl_kwargs['markerfirst'] == "Left of Entries"

    return mpl_kwargs


def _remove_kwargs_if_default(kwargs):
    """
    Remove kwargs from the given dict if they're the default values
    """
    for kwarg, default_value in mpl_default_kwargs.items():
        if kwargs[kwarg] == default_value:
            kwargs.pop(kwarg)

    # Font size defaults are string values (e.g. 'medium', 'large', 'x-large'), so we need to convert the defaults to
    # point sizes before comparing.
    if 'title_size' in kwargs:
        if convert_to_point_size(kwargs['title_size']) == convert_to_point_size(mpl_default_kwargs['title_size']):
            kwargs.pop('title_size')

    if 'entries_size' in kwargs:
        if convert_to_point_size(kwargs['entries_size']) == convert_to_point_size(mpl_default_kwargs['entries_size']):
            kwargs.pop('entries_size')

    # Hex values of colours may not be the same case, so convert to lower before comparing.
    if 'background_color' in kwargs:
        if kwargs['background_color'].lower() == mpl_default_kwargs['background_color'].lower():
            kwargs.pop('background_color')

    if 'edge_color' in kwargs:
        if kwargs['edge_color'].lower() == mpl_default_kwargs['edge_color'].lower():
            kwargs.pop('edge_color')


def convert_to_point_size(font_size):
    """
    Convert font size (may be int or string, e.g. 'medium', 'large', ...) to point size.
    """
    font = FontProperties()
    font.set_size(font_size)
    return font.get_size_in_points()
