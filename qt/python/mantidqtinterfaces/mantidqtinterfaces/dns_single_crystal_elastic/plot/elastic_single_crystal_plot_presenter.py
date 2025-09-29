# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS single crystal elastic plot tab presenter of DNS reduction GUI.
"""

import numpy as np
from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot import mpl_helpers
from mantidqtinterfaces.dns_single_crystal_elastic.plot.grid_locator import get_grid_helper
from mantid.simpleapi import logger


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
        self._plot_param.pointsize = 2
        self._plot_param.xlim = [None, None]
        self._plot_param.ylim = [None, None]
        self._plot_param.zlim = [None, None]
        self._plot_param.projections = False
        self._plot_param.use_default_lims = True

    def _toggle_projections(self, proj_on):
        self._plot_param.projections = proj_on
        if proj_on and self.model.has_data():
            x_proj, y_proj = self._calculate_projections()
            xlim, ylim, dummy_z = self._get_current_spinners_lims()
            self.view.single_crystal_plot.set_projections(x_proj, y_proj, xlim, ylim)
            self.view.draw()
        else:
            self.view.single_crystal_plot.remove_projections()
            self._plot(self.view.initial_values)

    def _calculate_projections(self):
        xlim, ylim = self.view.single_crystal_plot.get_active_limits()
        x_proj, y_proj = self.model.get_projections(xlim, ylim)
        return x_proj, y_proj

    def _datalist_updated(self, workspaces):
        compare = self.view.datalist.get_datalist()
        return (
            self.param_dict["elastic_single_crystal_script_generator"]["script_number"] != self._plotted_script_number
            or workspaces != compare
        )  # check is necessary for simulation

    def _plot(self, initial_values=None):
        """
        It is called every time any of the plot view options is changed
        or another polarization channel is selected for plotting.
        """
        plot_list = self.view.datalist.get_checked_plots()
        if not plot_list:
            return
        self._plot_param.plot_name = plot_list[0]
        self._change_font_size(draw=False)
        generated_dict = self.param_dict["elastic_single_crystal_script_generator"]
        data_array = generated_dict["data_arrays"][self._plot_param.plot_name]
        options = self.param_dict["elastic_single_crystal_options"]
        single_crystal_map = self.model.create_single_crystal_map(data_array, options, initial_values)
        self._determine_plot_type_options(single_crystal_map)
        self._change_grid_state(draw=False, change=False)
        self.view.create_subfigure(self._plot_param.grid_helper)
        plot_type = self.view.get_plotting_setting("plot_type")
        self._want_plot(plot_type)
        self._set_plotting_grid(self._crystallographical_axes())
        self._set_aspect_ratio()
        self._set_ax_formatter()
        self._set_axis_labels()
        self._set_log()
        self.view.single_crystal_plot.create_colorbar()
        self.view.connect_resize()
        self.view.single_crystal_plot.on_resize()
        self._set_initial_omega_offset_dx_dy()
        self.view.single_crystal_plot.connect_ylim_change()
        def_xlim, def_ylim, def_zlim = self.model.get_default_data_lims()
        xlim, ylim, zlim = self._get_current_spinners_lims()
        if self._plot_param.use_default_lims:
            self._set_plot_param_lims(def_xlim, def_ylim, def_zlim)
        else:
            self._set_plot_param_lims(xlim, ylim, zlim)
        self._set_spinners_lims(include_zlim=True)
        self._set_plotting_lims()
        if self._plot_param.projections:
            self._toggle_projections(proj_on=True)
        self.view.canvas.figure.tight_layout()
        self.view.draw()

    def _set_plot_param_lims(self, xlim, ylim, zlim):
        self._plot_param.xlim = xlim
        self._plot_param.ylim = ylim
        self._plot_param.zlim = zlim

    def _update_plot_param_lims(self, include_zlim=False):
        xlim, ylim = self.view.single_crystal_plot.get_active_limits()
        self._plot_param.xlim = xlim
        self._plot_param.ylim = ylim
        if include_zlim:
            zlim = self.model.get_data_z_min_max(xlim, ylim)
            self._plot_param.zlim = zlim

    def _set_plotting_lims(self):
        self.view.single_crystal_plot.set_xlim(self._plot_param.xlim)
        self.view.single_crystal_plot.set_ylim(self._plot_param.ylim)
        self.view.single_crystal_plot.set_zlim(self._plot_param.zlim)

    def process_auto_reduction_request(self):
        self.view.single_crystal_plot.clear_plot()

    def tab_got_focus(self):
        workspaces = sorted(self.param_dict["elastic_single_crystal_script_generator"]["plot_list"])
        if self._datalist_updated(workspaces):
            self.view.datalist.set_datalist(workspaces)
            self._plotted_script_number = self.param_dict["elastic_single_crystal_script_generator"]["script_number"]
            self.view.process_events()
            self.view.datalist.check_first()

    def _set_zooming_data(self, include_zlim=False):
        self._update_plot_param_lims(include_zlim)
        self._set_spinners_lims(include_zlim)
        self._set_plotting_lims()
        if self._plot_param.projections:
            self._toggle_projections(proj_on=True)
        # set this flag to false, not to update plot lims on a subsequent call of _plot()
        self._plot_param.use_default_lims = False

    def _set_initial_omega_offset_dx_dy(self):
        omega_offset = self.model.get_omega_offset()
        dx, dy = self.model.get_dx_dy()
        self.view.set_initial_omega_offset_dx_dy(omega_offset, dx, dy)

    def _update_omega_offset(self, omega_offset):
        self.view.initial_values["omega_offset"] = omega_offset
        self._plot(self.view.initial_values)

    def _set_default_omega_offset(self):
        default_value = self.param_dict["elastic_single_crystal_options"]["omega_offset"]
        self.view.initial_values["omega_offset"] = default_value
        self._plot(self.view.initial_values)

    def _update_dx_dy(self, dx, dy):
        self.view.initial_values["dx"] = dx
        self.view.initial_values["dy"] = dy
        self._plot(self.view.initial_values)

    def _set_default_dx_dy(self):
        default_dx = self.param_dict["elastic_single_crystal_options"]["dx"]
        default_dy = self.param_dict["elastic_single_crystal_options"]["dy"]
        self.view.initial_values["dx"] = default_dx
        self.view.initial_values["dy"] = default_dy
        self._plot(self.view.initial_values)

    def _plot_triangulation(self, interpolate, axis_type, switch):
        color_map, edge_colors, shading = self._get_plot_styles()
        triangulation, z = self.model.generate_triangulation_mesh(interpolate, axis_type, switch)
        self.view.single_crystal_plot.plot_triangulation(triangulation, z, color_map, edge_colors, shading)

    def _plot_quadmesh(self, interpolate, axis_type, switch):
        color_map, edge_colors, shading = self._get_plot_styles()
        if shading == "flat":  # prevents dropping of line
            shading = "nearest"
        x, y, z = self.model.generate_quad_mesh(interpolate, axis_type, switch)
        self.view.single_crystal_plot.plot_quadmesh(x, y, z, color_map, edge_colors, shading)

    def _plot_scatter(self, axis_type, switch):
        color_map = self._get_plot_styles()[0]
        x, y, z = self.model.generate_scatter_mesh(axis_type, switch)
        self.view.single_crystal_plot.plot_scatter(x, y, z, color_map)

    def _want_plot(self, plot_type):
        plot_settings = self.view.get_plotting_settings_dict()
        if plot_type == "triangulation":
            self._plot_triangulation(plot_settings["interpolate"], plot_settings["type"], plot_settings["switch"])
        if plot_type == "quadmesh":
            self._plot_quadmesh(plot_settings["interpolate"], plot_settings["type"], plot_settings["switch"])
        if plot_type == "scatter":
            self._plot_scatter(plot_settings["type"], plot_settings["switch"])

    def _get_plot_styles(self):
        own_dict = self.view.get_state()
        shading = self.view.get_plotting_setting("shading")
        edge_colors = ["face", "white", "black"][self._plot_param.lines]
        colormap_name = own_dict["colormap"]
        if own_dict["invert_cb"]:
            colormap_name += "_r"
        color_map = mpl_helpers.get_cmap(colormap_name)
        return color_map, edge_colors, shading

    def _set_axis_labels(self):
        axis_type = self.view.get_plotting_setting("type")
        switch = self.view.get_plotting_setting("switch")
        x_label, y_label = self.model.get_axis_labels(axis_type, self._crystallographical_axes())
        if switch:
            x_label, y_label = y_label, x_label
        self.view.single_crystal_plot.set_axis_labels(x_label, y_label)

    def _set_aspect_ratio(self):
        plot_settings = self.view.get_plotting_settings_dict()
        ratio = self.model.get_aspect_ratio(plot_settings)
        self.view.single_crystal_plot.set_aspect_ratio(ratio)

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
        self._plot(self.view.initial_values)

    def _change_line_style(self):
        plot_type = self.view.get_plotting_setting("plot_type")
        if plot_type == "scatter":
            self._plot_param.pointsize = (self._plot_param.pointsize + 1) % 5
            self.view.single_crystal_plot.set_pointsize(self._plot_param.pointsize)
        else:
            self._plot_param.lines = (self._plot_param.lines + 1) % 3
            self.view.single_crystal_plot.set_linecolor(self._plot_param.lines)
        self.view.draw()

    def _set_ax_formatter(self):
        switch = self.view.get_plotting_setting("switch")
        plot_settings = self.view.get_plotting_settings_dict()
        format_coord = self.model.get_format_coord(plot_settings, switch)
        self.view.single_crystal_plot.set_format_coord(format_coord)

    def _manual_lim_changed(self):
        xlim, ylim, zlim = self._get_current_spinners_lims()
        self._set_plot_param_lims(xlim, ylim, zlim)
        self._set_plotting_lims()
        self.view.draw()
        # set this flag to false, not to update plot lims on a subsequent call of _plot()
        self._plot_param.use_default_lims = False

    def _change_color_bar_range(self, zoom=True):
        xlim, ylim, zlim, _dummy = self._get_current_limits(zoom)
        self.view.single_crystal_plot.set_zlim(zlim)
        if zoom:  # saving zoom state to apply to all plots
            self._plot_param.xlim = xlim
            self._plot_param.ylim = ylim
            self._plot_param.zlim = zlim
            self._plot_param.sny_zoom_in = self.view.datalist.get_checked_plots()
        if zoom:
            self.view.draw()

    def _home_button_clicked(self):
        self.view.single_crystal_plot.disconnect_ylim_change()
        # The color bar gets tiny if you zoom out with home button in log scale.
        # Redrawing color bar fixes the problem.
        own_dict = self.view.get_state()
        plot_settings = self.view.get_plotting_settings_dict()
        if own_dict["log_scale"]:
            self.view.single_crystal_plot.redraw_colorbar()
        if plot_settings["zoom"]["fix_xy"]:
            dx_lim, dy_lim = self.model.get_data_xy_lim(plot_settings["switch"])
            self.view.single_crystal_plot.set_xlim(dx_lim)
            self.view.single_crystal_plot.set_ylim(dy_lim)
            self._plot_param.xlim = dx_lim
            self._plot_param.ylim = dy_lim
            self._change_color_bar_range(zoom=True)
        self.view.single_crystal_plot.connect_ylim_change()
        def_xlim, def_ylim, def_zlim = self.model.get_default_data_lims()
        self._set_plot_param_lims(def_xlim, def_ylim, def_zlim)
        self._set_spinners_lims(include_zlim=True)
        self._set_plotting_lims()
        if self._plot_param.projections:
            self._toggle_projections(proj_on=True)
        self.view.draw()

    def _get_current_xy_lim(self, zoom=False):
        own_dict = self.view.get_state()
        plot_settings = self.view.get_plotting_settings_dict()
        xlim, ylim = [[own_dict["x_min"], own_dict["x_max"]], [own_dict["y_min"], own_dict["y_max"]]]
        if zoom:
            dx_lim, dy_lim = self.view.single_crystal_plot.get_active_limits()
        else:
            dx_lim, dy_lim = self.model.get_data_xy_lim(plot_settings["switch"])
        if xlim[0] is None:
            xlim = dx_lim
        if ylim[0] is None:
            ylim = dy_lim
        if plot_settings["zoom"]["fix_xy"] and not zoom:
            if self._plot_param.xlim[0] is not None:
                xlim = self._plot_param.xlim
            if self._plot_param.ylim[0] is not None:
                ylim = self._plot_param.ylim
        return xlim, ylim

    def _get_current_z_lim(self, xlim, ylim, zoom):
        manual_z = True
        own_dict = self.view.get_state()
        plot_settings = self.view.get_plotting_settings_dict()
        if plot_settings["switch"]:
            xlim, ylim = ylim, xlim
        dz_min, dz_max, dpz_min = self.model.get_data_z_min_max(xlim, ylim)
        zlim = [own_dict["z_min"], own_dict["z_max"]]
        if zlim[0] is None:
            zlim = [dz_min, dz_max]
            manual_z = False
        if own_dict["log_scale"] and zlim[0] < 0:
            zlim[0] = dpz_min
        if plot_settings["zoom"]["fix_z"] and not zoom:
            if self._plot_param.zlim[0] is not None:
                zlim = self._plot_param.zlim
        return zlim, manual_z

    def _get_current_limits(self, zoom=True):
        x_lim, y_lim = self._get_current_xy_lim(zoom)
        z_lim, manual_z = self._get_current_z_lim(x_lim, y_lim, zoom)
        return x_lim, y_lim, z_lim, manual_z

    def _save_data(self):
        export_dir = self.param_dict["paths"]["export_dir"]
        displayed_ws_name = self._plot_param.plot_name
        export_file_name = f"{export_dir}/user_export_{displayed_ws_name}.csv"
        displayed_data = self.model.prepare_data_for_saving()
        column_headers = self._get_column_headers()
        data_table = np.concatenate((column_headers, displayed_data), axis=0)
        np.savetxt(export_file_name, data_table, delimiter=",", fmt="%s")
        self.view.show_status_message(f"Displayed data have been saved to: {export_file_name}", 10, clear=True)

    def _get_column_headers(self):
        axis_labels = self.view.get_plotting_settings_dict()["type"]
        if axis_labels == "qxqy":
            column_headers = np.array([["q_x (1/A)", "q_y (1/A)", "Intensity"]])
        elif axis_labels == "hkl":
            column_headers = np.array([["n_x", "n_y", "Intensity"]])
        elif axis_labels == "angular":
            column_headers = np.array([["2\u03b8 (deg)", "\u03c9 (deg)", "Intensity"]])
        return column_headers

    def _create_grid_helper(self):
        switch = self.view.get_plotting_setting("switch")
        a, b, c, d = self.model.get_changing_hkl_components()
        return get_grid_helper(self._plot_param.grid_helper, self._plot_param.grid_state, a, b, c, d, switch)

    def _set_colormap(self):
        cmap = self._get_plot_styles()[0]
        self.view.single_crystal_plot.set_cmap(cmap)
        self.view.draw()

    def _set_log(self):
        log = self.view.get_state()["log_scale"]
        _dummy, _dummy, zlim, _dummy = self._get_current_limits(zoom=True)
        norm = mpl_helpers.get_log_norm(log, zlim)
        self.view.single_crystal_plot.set_norm(norm)
        self.view.draw()

    def _change_font_size(self, draw=True):
        own_dict = self.view.get_state()
        font_size = own_dict["fontsize"]
        if self._plot_param.font_size != font_size:
            self._plot_param.font_size = font_size
            self.view.single_crystal_plot.set_fontsize(font_size)
            if draw:
                self._plot(self.view.initial_values)

    def _crystallographical_axes(self):
        own_dict = self.view.get_state()
        return own_dict["crystal_axes"]

    def _determine_plot_type_options(self, sc_map):
        if not sc_map.rectangular_grid:
            self.view.views_menu.menus[0].action_quad_mesh.setEnabled(False)
            if self.view.views_menu.menus[0].action_quad_mesh.isChecked():
                logger.warning(
                    "Warning: quadmesh only possible on a rectangular grid. Selected data do not compose "
                    "a rectangular grid. The selected plot type is changed to triangulation."
                )
                self.view.views_menu.menus[0].action_triangulation_mesh.setChecked(True)
        else:
            self.view.views_menu.menus[0].action_quad_mesh.setEnabled(True)

    def _get_current_spinners_lims(self):
        own_dict = self.view.get_state()
        xlim = [own_dict["x_min"], own_dict["x_max"]]
        ylim = [own_dict["y_min"], own_dict["y_max"]]
        zlim = [own_dict["z_min"], own_dict["z_max"]]
        return xlim, ylim, zlim

    def _set_spinners_lims(self, include_zlim=False):
        self.view._map["x_min"].setValue(self._plot_param.xlim[0])
        self.view._map["x_max"].setValue(self._plot_param.xlim[1])
        self.view._map["y_min"].setValue(self._plot_param.ylim[0])
        self.view._map["y_max"].setValue(self._plot_param.ylim[1])
        if include_zlim:
            self.view._map["z_min"].setValue(self._plot_param.zlim[0])
            self.view._map["z_max"].setValue(self._plot_param.zlim[1])

    def _switch_spinners_lims(self):
        xlim, ylim, dummy = self._get_current_spinners_lims()
        self.view._map["x_min"].setValue(ylim[0])
        self.view._map["x_max"].setValue(ylim[1])
        self.view._map["y_min"].setValue(xlim[0])
        self.view._map["y_max"].setValue(xlim[1])
        self._plot()
        if self._plot_param.projections:
            self._toggle_projections(proj_on=True)

    def _change_axes(self):
        self._plot_param.use_default_lims = True
        self._plot()
        if self._plot_param.projections:
            self._toggle_projections(proj_on=True)

    def _attach_signal_slots(self):
        self.view.sig_plot.connect(self._plot)
        self.view.sig_update_omega_offset.connect(self._update_omega_offset)
        self.view.sig_restore_default_omega_offset.connect(self._set_default_omega_offset)
        self.view.sig_update_dxdy.connect(self._update_dx_dy)
        self.view.sig_restore_default_dxdy.connect(self._set_default_dx_dy)
        self.view.sig_calculate_projection.connect(self._toggle_projections)
        self.view.sig_save_data.connect(self._save_data)
        self.view.sig_change_colormap.connect(self._set_colormap)
        self.view.sig_change_log.connect(self._set_log)
        self.view.sig_change_linestyle.connect(self._change_line_style)
        self.view.sig_change_cb_range_on_zoom.connect(self._change_color_bar_range)
        self.view.sig_manual_lim_changed.connect(self._manual_lim_changed)
        self._plotted_script_number = 0
        self.view.sig_change_grid.connect(self._change_grid_state)
        self.view.sig_change_crystal_axes.connect(self._change_crystal_axes)
        self.view.sig_change_font_size.connect(self._change_font_size)
        self.view.sig_home_button_clicked.connect(self._home_button_clicked)
        self.view.sig_plot_zoom_updated.connect(self._set_zooming_data)
        self.view.sig_switch_changed.connect(self._switch_spinners_lims)
        self.view.sig_axes_changed.connect(self._change_axes)
