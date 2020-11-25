# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
#
import itertools
from collections import defaultdict

import matplotlib as mpl

from mantidqt.project.projectreaderinterface import ProjectReaderInterface
from mantid import logger
import re


def get_settings_from_tag_group(tag_text):
    settings = defaultdict(list)
    lines = tag_text.split('\n')
    for line in lines:
        separated_line = line.split('\t')
        setting = separated_line[0]
        value = [item.strip() for item in itertools.islice(separated_line, 1, None)]
        settings[setting].append(value)
    return settings


def create_workbench_creation_args(plot_settings):
    workspace_settings = plot_settings["MantidMatrixCurve"]
    plot_creation_args = []
    for workspace_setting in workspace_settings:
        creation_args = {"linestyle": "solid", "linewidth": 1.5, "drawstyle": "default", "marker": None,
                         "markersize": 6,
                         "function": "plot"}
        workspace = workspace_setting[0]
        # Mantid project files contain the workspace index
        sp = workspace_setting[workspace_setting.index("sp") + 1]
        creation_args["wkspIndex"] = int(sp)
        creation_args["workspaces"] = workspace
        plot_creation_args.append(creation_args)
    return plot_creation_args


def create_workbench_line_args(plot_settings):
    workspace_settings = plot_settings["MantidMatrixCurve"]
    number_of_lines = len(workspace_settings)
    line_args = []
    for i in range(number_of_lines):
        line_arg = {}
        line_arg["lineIndex"] = 0
        line_arg["markerStyle"] = {}
        if i == 1:
            line_arg["errorbars"] = {}
            line_arg["errorbars"]["exists"] = True
        else:
            line_arg["errorbars"] = {}
            line_arg["errorbars"]["exists"] = False
        line_args.append(line_arg)

    return line_args


def create_workbench_legend_args(plot_settings):
    legend_settings = plot_settings.get('<legend>', None)
    exists = bool(legend_settings)
    visible = bool(legend_settings)
    return {'exists': exists, 'visible': visible, 'title': '', 'title_font': 'DejaVu Sans', 'title_size': 10.0,
            'title_color': '#000000', 'box_visible': True, 'background_color': '#ffffff',
            'edge_color': '#cccccc',
            'transparency': 0.8, 'entries_font': 'DejaVu Sans', 'entries_size': 8.0, 'entries_color': '#000000',
            'marker_size': 2.0, 'shadow': False, 'round_edges': True, 'columns': 1, 'column_spacing': 2.0,
            'label_spacing': 0.5, 'marker_position': 'Left of Entries', 'markers': 1, 'border_padding': 0.4,
            'marker_label_padding': 0.8}


def create_workbench_axes_args(plot_settings):
    axes_args = {"title": plot_settings.get("PlotTitle", [None])[0][0]}
    axes_titles = plot_settings.get("AxesTitles", None)[0]
    axes_args["xAxisTitle"] = axes_titles[0] if axes_titles else None
    axes_args["yAxisTitle"] = axes_titles[1] if axes_titles else None
    line_args = create_workbench_line_args(plot_settings)
    # create sub dict of axes properties
    axes_args["properties"] = create_workbench_axes_properties(plot_settings)
    axes_args["lines"] = line_args
    axes_args['legend'] = create_workbench_legend_args(plot_settings)
    return axes_args


def create_workbench_axes_properties(plot_settings):
    properties = {}

    # we cannot set the bounds as it relates to the matplotlib canvas axis position
    properties["bounds"] = None
    properties["dynamic"] = True
    properties["axisOn"] = True
    properties["frameOn"] = True
    properties["visible"] = True

    axisbounds = plot_settings["scale"]
    properties["yLim"] = [float(axisbounds[0][1]), float(axisbounds[0][2])]
    properties["xLim"] = [float(axisbounds[2][1]), float(axisbounds[2][2])]
    properties["xAxisScale"] = "linear"
    properties["yAxisScale"] = "linear"
    properties["showMinorGrid"] = False

    properties["colorbar"] = {}
    properties["colorbar"]["exists"] = False

    properties["title"] = plot_settings["PlotTitle"][0][0]
    properties["xAxisProperties"] = {'majorTickLocator': 'AutoLocator', 'minorTickLocator': 'NullLocator',
                                     'majorTickFormatter': 'ScalarFormatter', 'minorTickFormatter': 'NullFormatter',
                                     'gridStyle': {'gridOn': False},
                                     'visible': True, 'position': 'Bottom', 'majorTickLocatorValues': None,
                                     'minorTickLocatorValues': None, 'majorTickFormat': None, 'minorTickFormat': None,
                                     'fontSize': 10.0}

    properties["yAxisProperties"] = {'majorTickLocator': 'AutoLocator', 'minorTickLocator': 'NullLocator',
                                     'majorTickFormatter': 'ScalarFormatter', 'minorTickFormatter': 'NullFormatter',
                                     'gridStyle': {'gridOn': False},
                                     'visible': True, 'position': 'Bottom', 'majorTickLocatorValues': None,
                                     'minorTickLocatorValues': None, 'majorTickFormat': None, 'minorTickFormat': None,
                                     'fontSize': 10.0}

    return properties


def get_plot_label_from_layer(layer_settings):
    # label for the layer is defined as the first item in the first line of the layer settings
    label, _ = layer_settings.split(maxsplit=1)
    return label


def load_default_fig_properties():
    fig_size = mpl.rcParams['figure.figsize']
    return {'figWidth': fig_size[0],
            'figHeight': fig_size[1],
            'dpi': mpl.rcParams['figure.dpi']}


class ProjectReaderMantidPlot(ProjectReaderInterface):

    def __init__(self, project_file_ext, filename):
        super().__init__(project_file_ext, filename)
        self.full_text = []

    def read_project(self):
        try:
            with open(self.filename, encoding="utf8") as f:
                self.full_text = f.read()
                self.read_workspaces()
                self.read_interfaces()
                self.read_plots()
        except Exception as err:
            raise
            logger.warning("Mantidplot project file unable to be loaded/read" + err)

    def read_workspaces(self):
        # Get the string inside the mantidworkspaces tags, allowing for whitespace at either end
        workspaces_pattern = r"<mantidworkspaces>\s*(.*?)\s*<\/mantidworkspaces>"
        ws_match = re.search(workspaces_pattern, self.full_text, re.DOTALL)
        if ws_match:
            # split by tab
            ws_list = ws_match.group(1).split('\t')
            if len(ws_list) > 1 and ws_list[0] == "WorkspaceNames":
                # the first entry is just an identification tag
                self.workspace_names = ws_list[1:]
                logger.notice("Loading workspaces from Mantidplot project file " + self.filename)

    def read_interfaces(self):
        logger.notice("Loading interfaces from mantid plot project file not supported")

    def read_plots(self):
        plot_layer_pattern = r"<multiLayer>\s*(.*?)\s*<\/multiLayer>"
        graph_pattern = r"<graph>\s*(.*?)\s*<\/graph>"
        layers = re.findall(plot_layer_pattern, self.full_text, re.DOTALL)
        if not layers:
            return
        self.plot_list = []
        for layer in layers:
            logger.notice("Loading plots from MantidPlot project file")
            creation_args = {'creationArguments': [], 'axes': [],
                             'label': get_plot_label_from_layer(layer),
                             'properties': load_default_fig_properties()}
            graphs = re.findall(graph_pattern, layer, re.DOTALL)
            for graph in graphs:
                graph_settings = get_settings_from_tag_group(graph)
                graph_args = create_workbench_creation_args(graph_settings)
                axes_args = create_workbench_axes_args(graph_settings)
                creation_args["creationArguments"].append(graph_args)
                creation_args["axes"].append(axes_args)
            self.plot_list.append(creation_args)
