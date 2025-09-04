# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab presenter of DNS reduction GUI.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot import mpl_helpers
from mantidqtinterfaces.dns_single_crystal_elastic.plot.grid_locator import get_grid_helper


class DNSElasticSCPlotPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        # connect signals
        self._attach_signal_slots()
        # plot parameter
        self._plot_param = ObjectDict()
        self._plot_param.grid_state = 0
        self._plot_param.grid_helper = None
        self._plot_param.colormap_name = "jet"
        self._plot_param.font_size = 1
        self._plot_param.lines = 0

    def _datalist_updated(self, workspaces):
        compare = self.view.datalist.get_datalist()
        return (
            self.param_dict["elastic_single_crystal_script_generator"]["script_number"] != self._plotted_script_number
            or workspaces != compare
        )  # check is necessary for simulation

    def _plot(self, initial_values=None):
        axis_type = self.view.get_axis_type()
        plot_list = self.view.datalist.get_checked_plots()
        if not plot_list:
            return
        self._plot_param.plot_name = plot_list[0]
        self._change_font_size(draw=False)
        generated_dict = self.param_dict["elastic_single_crystal_script_generator"]
        data_array = generated_dict["data_arrays"][self._plot_param.plot_name]
        options = self.param_dict["elastic_single_crystal_options"]
        self.model.create_single_crystal_map(data_array, options, initial_values)
        self._change_grid_state(draw=False, change=False)
        self.view.create_subfigure(self._plot_param.grid_helper)
        self._want_plot(axis_type["plot_type"])
        self._set_plotting_grid(self._crystallographical_axes())
        self._set_axis_labels()
        self.view.single_crystal_plot.create_colorbar()
        self.view.single_crystal_plot.on_resize()
        self.view.canvas.figure.tight_layout()
        self.view.draw()

    def tab_got_focus(self):
        workspaces = sorted(self.param_dict["elastic_single_crystal_script_generator"]["plot_list"])
        if self._datalist_updated(workspaces):
            self.view.datalist.set_datalist(workspaces)
            self._plotted_script_number = self.param_dict["elastic_single_crystal_script_generator"]["script_number"]
            self.view.process_events()
            self.view.datalist.check_first()

    def _plot_triangulation(self, interpolate, axis_type, switch):
        color_map, edge_colors, shading = self._get_plot_styles()
        triangulation, z = self.model.get_interpolated_triangulation(interpolate, axis_type, switch)
        self.view.single_crystal_plot.plot_triangulation(triangulation, z, color_map, edge_colors, shading)

    def _want_plot(self, plot_type):
        axis_type = self.view.get_axis_type()
        self._plot_triangulation(axis_type["interpolate"], axis_type["type"], axis_type["switch"])

    def _get_plot_styles(self):
        own_dict = self.view.get_state()
        shading = "flat"
        edge_colors = ["face", "white", "black"][self._plot_param.lines]
        colormap_name = own_dict["colormap"]
        if own_dict["invert_cb"]:
            colormap_name += "_r"
        cmap = mpl_helpers.get_cmap(colormap_name)
        return cmap, edge_colors, shading

    def _set_axis_labels(self):
        axis_type = self.view.get_axis_type()
        x_label, y_label = self.model.get_axis_labels(axis_type["type"], self._crystallographical_axes())
        self.view.single_crystal_plot.set_axis_labels(x_label, y_label)

    def _change_crystal_axes_grid(self):
        current_state = self._plot_param.grid_state % 4
        if current_state == 0:
            self._plot_param.grid_state = 1
        else:
            self._plot_param.grid_state = current_state
        self._set_plotting_grid(crystallographic_axes=True)

    def _change_normal_grid(self):
        self._plot_param.grid_state = self._plot_param.grid_state % 3
        self._set_plotting_grid(crystallographic_axes=False)

    def _set_plotting_grid(self, crystallographic_axes=True):
        if crystallographic_axes:
            self._plot_param.grid_helper = self._create_grid_helper()
            self.view.single_crystal_plot.set_grid(major=self._plot_param.grid_state, minor=self._plot_param.grid_state // 3)
        else:
            self._plot_param.grid_helper = None
            self.view.single_crystal_plot.set_grid(major=self._plot_param.grid_state, minor=self._plot_param.grid_state // 2)

    def _change_grid_state(self, draw=True, change=True):
        if change:
            self._plot_param.grid_state = self._plot_param.grid_state + 1
        if self._crystallographical_axes():
            self._change_crystal_axes_grid()
        else:
            self._change_normal_grid()
        if draw:
            self.view.draw()

    def _change_crystal_axes(self):
        if self._crystallographical_axes():
            self._plot_param.grid_state = 1
        else:
            self._plot_param.grid_state = 0
        self._plot()

    def _create_grid_helper(self):
        axis_type = self.view.get_axis_type()
        a, b, c, d = self.model.get_changing_hkl_components()
        return get_grid_helper(self._plot_param.grid_helper, self._plot_param.grid_state, a, b, c, d, axis_type["switch"])

    def _set_colormap(self):
        cmap = self._get_plot_styles()[0]
        self.view.single_crystal_plot.set_cmap(cmap)
        self.view.draw()

    def _change_font_size(self, draw=True):
        own_dict = self.view.get_state()
        font_size = own_dict["fontsize"]
        if self._plot_param.font_size != font_size:
            self._plot_param.font_size = font_size
            self.view.single_crystal_plot.set_fontsize(font_size)
            if draw:
                self._plot()

    def _crystallographical_axes(self):
        own_dict = self.view.get_state()
        return own_dict["crystal_axes"]

    def _attach_signal_slots(self):
        self.view.sig_plot.connect(self._plot)
        self.view.sig_change_colormap.connect(self._set_colormap)
        self._plotted_script_number = 0
        self.view.sig_change_grid.connect(self._change_grid_state)
        self.view.sig_change_crystal_axes.connect(self._change_crystal_axes)
        self.view.sig_change_font_size.connect(self._change_font_size)
