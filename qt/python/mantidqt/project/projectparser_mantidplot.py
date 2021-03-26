# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt package
import re
import matplotlib as mpl

SCALE_OPTIONS = {0: "linear", 1: "log"}


class MantidPlotProjectParser(object):
    plot_layer_pattern = r"<multiLayer>\s*(.*?)\s*<\/multiLayer>"
    graph_pattern = r"<graph>\s*(.*?)\s*<\/graph>"
    workspaces_pattern = r"<mantidworkspaces>\s*(.*?)\s*<\/mantidworkspaces>"

    def __init__(self, filename):
        self.filename = filename
        self.graph_text = []
        self.layer_text = []
        self.number_of_figures = 0
        self.number_of_graphs = []
        self._load_file_text()

    def _load_file_text(self):
        with open(self.filename, encoding="utf8") as project_file:
            self.text = project_file.read()
        self.layer_text = re.findall(self.plot_layer_pattern, self.text, re.DOTALL)
        self.number_of_figures = len(self.layer_text)
        self.number_of_graphs = [0] * self.number_of_figures

    def get_workspaces(self):
        """
        Get workspaces to load from project file
        """
        ws_match = re.search(self.workspaces_pattern, self.text, re.DOTALL)
        if ws_match:
            # split by tab
            mantid_workspace_entries = ws_match.group(1).split('\t')
            if len(mantid_workspace_entries) > 1 and mantid_workspace_entries[0] == "WorkspaceNames":
                # Load workspace groups
                return self._load_workspaces(mantid_workspace_entries[1:])
            else:
                return []

    def _load_workspaces(self, workspace_entries):
        """
        Identify workspaces within a <mantidworkspaces> tag
        :param workspace_entries: The entries within the mantidworkspaces tag
        :return list of workspaces found within the <mantidworkspaces> tag
        """
        ws_list = []
        ws_group_members = []
        for ws_entry in workspace_entries:
            # if the entry within the <mantidworkspaces> contains a "," it defines a workspace group
            if ',' in ws_entry:
                ws_group = ws_entry.split(',')
                # First entry is group name, remainder of the line contains members of the group
                ws_group_members.extend(ws_group[1:])
            else:
                if ws_entry not in ws_group_members:
                    ws_list.append(ws_entry)
        return ws_group_members + ws_list

    def get_plots(self):
        """
        Get plots to load from project file
        """
        plot_list = []
        for i in range(self.number_of_figures):
            plot_list.append(self._get_plot_creation_arguments(i))
        return plot_list

    def _get_plot_creation_arguments(self, layer_index):
        """
        Load plot creation arguments from a particular MantidPlot figure layer
        :param layer_index: Layer index where the plot creation arguments will be retrieved
        """
        if layer_index > self.number_of_figures:
            return
        graph_creation_args = self._get_graph_creation_arguments_in_layer(layer_index)
        axes_creation_args = self._get_axes_creation_arguments(layer_index)
        layer_label = self._get_plot_label_from_layer(layer_index)
        layer_creation_args = {
            'creationArguments': graph_creation_args,
            'axes': axes_creation_args,
            'label': layer_label,
            'properties': self._get_default_fig_properties()
        }
        return layer_creation_args

    def _get_axes_creation_arguments(self, layer_index):
        """
        Load axes creation properties from a particular MantidPlot figure layer
        :param layer_index: Layer index where the axes properties will be retrieved
        """
        axes_creation_arguments = []
        layer_text = self.layer_text[layer_index]
        graph_texts = re.findall(self.graph_pattern, layer_text, re.DOTALL)
        for graph_text in graph_texts:
            axes_args = {}
            _, title_line = self.find_option_in_raw_text("PlotTitle", graph_text)[0]
            axes_args["title"] = title_line[1] if title_line else None
            _, axes_titles = self.find_option_in_raw_text("AxesTitles", graph_text)[0]
            axes_args["xAxisTitle"] = axes_titles[1] if axes_titles else None
            axes_args["yAxisTitle"] = axes_titles[2] if axes_titles else None
            axes_args["lines"] = self._get_line_creation_arguments_from_graph_entry(graph_text)
            axes_args["properties"] = self._get_axes_properties_from_graph_entry(graph_text)
            axes_args['legend'] = self._get_legend_properties_from_graph_entry(graph_text)
            axes_creation_arguments.append(axes_args)
        return axes_creation_arguments

    def _get_plot_label_from_layer(self, layer_index):
        """
        Load figure label at a particular layer_index
        :param layer_index: Layer index where the label will be retrieved
        """
        # label for the layer is defined as the first item in the first line of the layer settings
        layer_text = self.layer_text[layer_index]
        label = layer_text.split('\n', maxsplit=1)[0].split('\t')[0]
        return label

    def _get_graph_creation_arguments_in_layer(self, layer_index):
        """
        Get the creation settings for the graph, looks for each <graph></graph> entry within
        each layer (with index=index) in the MantidProject file.
        :param layer_index: Layer index where the graphs will be retrieved
        """
        layer_text = self.layer_text[layer_index]
        graphs = re.findall(self.graph_pattern, layer_text, re.DOTALL)
        self.number_of_graphs[layer_index] = len(graphs)
        graph_creation_args = []
        for graph in graphs:
            creation_args = self._get_1d_graph_creation_args_from_graph_entry(graph)
            graph_creation_args.append(creation_args)
        return graph_creation_args

    def _get_line_creation_arguments_from_graph_entry(self, graph_text):
        """
        Load line properties from input graph text
        :param graph_text: Text contained within a <graph> tag
        """
        curve_entries = self.find_option_in_raw_text("MantidMatrixCurve", graph_text)
        number_of_lines = len(curve_entries)
        line_creation_arguments = []
        for i in range(number_of_lines):
            line_arg = {"lineIndex": i, "markerStyle": {}, "errorbars": {}}
            line_arg["errorbars"]["exists"] = False
            line_creation_arguments.append(line_arg)

        return line_creation_arguments

    def _get_axes_properties_from_graph_entry(self, graph_text):
        """
        Load axes properties from input graph text
        :param graph_text: Text contained within a <graph> tag
        """
        # Default settings
        properties = {"bounds": None, "dynamic": True, "axisOn": True, "frameOn": True, "visible": True}
        # scale
        scale_lines = self.find_option_in_raw_text("scale", graph_text)
        ybounds = scale_lines[0][1]
        xbounds = scale_lines[2][1]
        properties["xLim"] = [float(xbounds[2]), float(xbounds[3])]
        properties["yLim"] = [float(ybounds[2]), float(ybounds[3])]
        try:
            xscale = SCALE_OPTIONS[int(float(xbounds[7]))]
            yscale = SCALE_OPTIONS[int(float(ybounds[7]))]
        except KeyError:
            raise RuntimeError("Cannot load axis scale, powerscale axes are not supported")
        properties["xAxisScale"] = xscale
        properties["yAxisScale"] = yscale
        properties["showMinorGrid"] = False
        # colorbar
        properties["colorbar"] = {}
        properties["colorbar"]["exists"] = False
        # title
        _, plot_title = self.find_option_in_raw_text("PlotTitle", graph_text)[0]
        properties["title"] = plot_title[1] if plot_title else None
        # axesproperties
        default_axis_properties = {
            'majorTickLocator': 'AutoLocator',
            'minorTickLocator': 'NullLocator',
            'majorTickFormatter': 'ScalarFormatter',
            'minorTickFormatter': 'NullFormatter',
            'gridStyle': {
                'gridOn': False
            },
            'visible': True,
            'position': 'Bottom',
            'majorTickLocatorValues': None,
            'minorTickLocatorValues': None,
            'majorTickFormat': None,
            'minorTickFormat': None,
            'fontSize': 10.0
        }
        properties["xAxisProperties"] = default_axis_properties
        properties["yAxisProperties"] = default_axis_properties

        return properties

    def _get_legend_properties_from_graph_entry(self, graph_text):
        """
        Load legend properties from input graph tag
        :param graph_text: Text contained within a <graph> tag
        """
        _, legend_entry = self.find_option_in_raw_text("<legend>", graph_text)[0]
        exists = bool(legend_entry)
        visible = bool(legend_entry)
        return {
            'exists': exists,
            'visible': visible,
            'title': '',
            'title_font': 'DejaVu Sans',
            'title_size': 10.0,
            'title_color': '#000000',
            'box_visible': True,
            'background_color': '#ffffff',
            'edge_color': '#cccccc',
            'transparency': 0.8,
            'entries_font': 'DejaVu Sans',
            'entries_size': 8.0,
            'entries_color': '#000000',
            'marker_size': 2.0,
            'shadow': False,
            'round_edges': True,
            'columns': 1,
            'column_spacing': 2.0,
            'label_spacing': 0.5,
            'marker_position': 'Left of Entries',
            'markers': 1,
            'border_padding': 0.4,
            'marker_label_padding': 0.8
        }

    def _get_1d_graph_creation_args_from_graph_entry(self, graph_text):
        """
        Get the creation settings for 1d graphs, looking within the <MantidMatrixCurve> tag
        :param graph_text: Text contained within a <graph> tag
        """
        curve_entries = self.find_option_in_raw_text("MantidMatrixCurve", graph_text)
        curve_creation_args = []
        for line_number, curve_settings in curve_entries:
            # remove extraneous white space
            curve_settings = [entry.strip() for entry in curve_settings]
            creation_args = {
                "linestyle": "solid",
                "linewidth": 1.5,
                "drawstyle": "default",
                "marker": None,
                "markersize": 6,
                "function": self.get_graph_1d_plot_function(line_number, graph_text)
            }
            workspace = curve_settings[1]
            # Mantid project files contain the workspace index
            sp = curve_settings[curve_settings.index("sp") + 1]
            creation_args["wkspIndex"] = int(sp)
            creation_args["workspaces"] = workspace
            curve_creation_args.append(creation_args)
        return curve_creation_args

    @staticmethod
    def _get_default_fig_properties():
        fig_size = mpl.rcParams['figure.figsize']
        return {'figWidth': fig_size[0], 'figHeight': fig_size[1], 'dpi': mpl.rcParams['figure.dpi']}

    @staticmethod
    def find_option_in_raw_text(option, text):
        return [(i, line.split('\t')) for i, line in enumerate(text.split('\n')) if option == line.split(maxsplit=1)[0]]

    @staticmethod
    def get_graph_1d_plot_function(curve_entry_line_number, graph_text):
        if "<MantidYErrors>1" in graph_text.split('\n')[curve_entry_line_number + 1]:
            return "errorbar"
        else:
            return "plot"
