# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer \
    import DNSObserver
from mantidqtinterfaces.dns_powder_tof.data_structures.object_dict \
    import ObjectDict
from mantidqtinterfaces.dns_single_crystal_elastic.plot import mpl_helpers
from mantidqtinterfaces.dns_single_crystal_elastic.plot.grid_locator import get_gridhelper


class DNSElasticSCPlotPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.view.sig_plot.connect(self._plot)
        self.view.sig_update_omegaoffset.connect(self._update_omegaoffset)
        self.view.sig_update_dxdy.connect(self._update_dx_dy)
        self.view.sig_calculate_projection.connect(self._toggle_projections)
        # self.view.sig_save_data.connect(self.save_data)
        self.view.sig_change_colormap.connect(self._set_colormap)
        self.view.sig_change_log.connect(self._set_log)
        self.view.sig_change_linestyle.connect(self._change_linestyle)
        self.view.sig_change_cb_range_on_zoom.connect(self._change_cb_range)
        self.view.sig_manual_lim_changed.connect(self._manual_lim_changed)
        self._plotted_script_number = 0
        # self.app = get_qapplication()[0]
        self.view.sig_change_grid.connect(self._change_grid_state)
        self.view.sig_change_crystal_axes.connect(self._change_crystal_axes)
        self.view.sig_change_fontsize.connect(self._change_fontsize)
        self.view.sig_home_button_clicked.connect(self._home_button_clicked)

        # plot parameter
        self._plot_param = ObjectDict()
        self._plot_param.gridstate = 0
        self._plot_param.gridhelper = None
        self._plot_param.colormap_name = 'jet'
        self._plot_param.fontsize = 1
        self._plot_param.lines = 0
        self._plot_param.pointsize = 2
        self._plot_param.xlim = [None, None]
        self._plot_param.ylim = [None, None]
        self._plot_param.zlim = [None, None]
        self._plot_param.projections = False

    def _toggle_projections(self, set_proj):
        self.view.sig_change_cb_range_on_zoom.disconnect()
        self._plot_param.projections = set_proj
        axistype = self.view.get_axis_type()
        if set_proj and self.model.has_data():
            xproj, yproj = self._calculate_projections(axistype['switch'])
            if axistype['switch']:
                xproj, yproj = yproj, xproj
            self.view.single_crystal_plot.set_projections(xproj, yproj)
            self.view.draw()
        else:
            self.view.single_crystal_plot.remove_projections()
            self._plot()
        self.view.sig_change_cb_range_on_zoom.connect(self._change_cb_range)

    def _calculate_projections(self, switch=False):
        xlim, ylim = self.view.single_crystal_plot.get_active_limits()
        if switch:
            xlim, ylim = ylim, xlim
        xproj, yproj = self.model.get_projections(xlim, ylim)
        return xproj, yproj

    def _datalist_updated(self, workspaces):
        compare = self.view.datalist.get_datalist()
        return (self.param_dict['elastic_single_crystal_script_generator']['script_number']
                != self._plotted_script_number
                or workspaces != compare)  # check is nesesary for simulation

    def _plot(self, initial_values=None):
        axistype = self.view.get_axis_type()
        plotlist = self.view.datalist.get_checked_plots()
        if not plotlist:
            return
        self._plot_param.plotname = plotlist[0]
        self._change_fontsize(draw=False)
        gendict = self.param_dict['elastic_single_crystal_script_generator']
        data_array = gendict['data_arrays'][self._plot_param.plotname]
        options = self.param_dict['elastic_single_crystal_options']
        self.model.create_single_crystal_map(data_array, options, initial_values)
        self._change_grid_state(draw=False, change=False)
        self.view.create_subfigure(self._plot_param.gridhelper)
        self._want_plot(axistype['plot_type'])
        self._change_grid_state(draw=False, change=False)
        self._set_aspect_ratio()
        self._set_ax_formatter()
        self._set_axis_labels()
        self.view.single_crystal_plot.create_colorbar()
        self.view.connect_resize()
        self.view.single_crystal_plot.onresize()
        self._manual_lim_changed()
        self._set_initial_oof_dxdy()
        self.view.single_crystal_plot.connect_ylim_change()
        self.view.draw()

    def _set_initial_oof_dxdy(self):
        oof = self.model.get_omegaoffset()
        dx, dy = self.model.get_dx_dy()
        self.view.set_initial_oof_dxdy(oof, dx, dy)

    def process_auto_reduction_request(self):
        self.view.single_crystal_plot.clear_plot()

    def tab_got_focus(self):
        workspaces = sorted(self.param_dict['elastic_single_crystal'
                                            '_script_generator']['plotlist'])
        if self._datalist_updated(workspaces):
            self.view.datalist.set_datalist(workspaces)
            self._plotted_script_number = self.param_dict[
                'elastic_single_crystal_script_generator']['script_number']
            self.view.process_events()
            self.view.datalist.check_first()

    def _update_omegaoffset(self, omegaoffset):
        initial_values = {'omega_offset': omegaoffset}
        self._plot(initial_values)

    def _update_dx_dy(self, dx, dy):
        initial_values = {'dx': dx, 'dy': dy}
        self._plot(initial_values)

    def _plot_quadmesh(self, interpolate, atype, switch):
        cmap, edgecolors, shading = self._get_plot_styles()
        if shading == 'flat':  # prevents dropping of line
            shading = 'nearest'
        x, y, z = self.model.get_interpolated_quadmesh(
            interpolate, atype)
        x, y, z = self.model.switch_axis(x, y, z, switch)
        self.view.single_crystal_plot.plot_quadmesh(x, y, z, cmap, edgecolors, shading)

    def _plot_triangulation(self, interpolate, atype, switch):
        cmap, edgecolors, shading = self._get_plot_styles()
        triang, z = self.model.get_interpolated_triangulation(
            interpolate, atype, switch)
        self.view.single_crystal_plot.plot_triangulation(
            triang, z, cmap, edgecolors, shading)

    def _plot_scatter(self, atype, switch):
        cmap = self._get_plot_styles()[0]
        x, y, z = self.model.get_interpolated_quadmesh(False, atype)
        x, y, z = self.model.switch_axis(x, y, z, switch)
        self.view.single_crystal_plot.plot_scatter(x, y, z, cmap)

    def _want_plot(self, plot_type):
        axistype = self.view.get_axis_type()
        if plot_type == 'quadmesh':
            self._plot_quadmesh(axistype['interpolate'], axistype['type'],
                                axistype['switch'])
        if plot_type == 'triangulation':
            self._plot_triangulation(
                axistype['interpolate'], axistype['type'], axistype['switch'])
        if plot_type == 'scatter':
            self._plot_scatter(axistype['type'], axistype['switch'])

    def _get_plot_styles(self):
        own_dict = self.view.get_state()
        axistype = self.view.get_axis_type()
        shading = axistype['shading']
        edgecolors = ['face', 'white', 'black'][self._plot_param.lines]
        colormap_name = own_dict['colormap']
        if own_dict['invert_cb']:
            colormap_name += '_r'
        cmap = mpl_helpers.get_cmap(colormap_name)
        return cmap, edgecolors, shading

    def _set_axis_labels(self):
        axistype = self.view.get_axis_type()
        own_dict = self.view.get_state()
        xlabel, ylabel = self.model.get_axis_labels(axistype['type'],
                                                    own_dict['crystal_axes'])
        if axistype['switch']:
            xlabel, ylabel = ylabel, xlabel
        self.view.single_crystal_plot.set_axis_labels(xlabel, ylabel)

    def _set_aspect_ratio(self):
        axistype = self.view.get_axis_type()
        ratio = self.model.get_aspect_ratio(axistype)
        self.view.single_crystal_plot.set_aspect_ratio(ratio)

    def _change_crystal_axes_grid(self):
        self._plot_param.gridstate = self._plot_param.gridstate % 4
        self._plot_param.gridhelper = self._create_gridhelper()
        self.view.single_crystal_plot.set_grid(major=True)

    def _change_normal_grid(self):
        self._plot_param.gridstate = self._plot_param.gridstate % 3
        self._plot_param.gridhelper = None
        self.view.single_crystal_plot.set_grid(major=self._plot_param.gridstate,
                                               minor=self._plot_param.gridstate // 2)

    def _change_grid_state(self, draw=True, change=True):
        own_dict = self.view.get_state()
        if change:
            self._plot_param.gridstate = self._plot_param.gridstate + 1
        if own_dict['crystal_axes']:
            self._change_crystal_axes_grid()
        else:
            self._change_normal_grid()
        if draw:
            self.view.draw()

    def _change_crystal_axes(self):
        self._plot_param.gridstate = 1
        self._plot()

    def _create_gridhelper(self):
        axistype = self.view.get_axis_type()
        a, b, c, d = self.model.get_changing_hkl_components()
        return get_gridhelper(self._plot_param.gridhelper,
                              self._plot_param.gridstate, a, b, c, d,
                              axistype['switch'])

    def _set_colormap(self):
        cmap = self._get_plot_styles()[0]
        self.view.single_crystal_plot.set_cmap(cmap)
        self.view.draw()

    def _set_log(self):
        log = self.view.get_state()['log_scale']
        _dummy, _dummy, zlim, _dummy = self._get_current_limits(zoom=True)
        norm = mpl_helpers.get_log_norm(log, zlim)
        self.view.single_crystal_plot.set_norm(norm)
        self.view.draw()

    def _change_fontsize(self, draw=True):
        own_dict = self.view.get_state()
        fontsize = own_dict['fontsize']
        if self._plot_param.fontsize != fontsize:  #
            self._plot_param.fontsize = fontsize
            self.view.single_crystal_plot.set_fontsize(fontsize)
            if draw:
                self._plot()

    def _change_linestyle(self):
        axistype = self.view.get_axis_type()
        if axistype['plot_type'] == 'scatter':
            self._plot_param.pointsize = (self._plot_param.pointsize + 1) % 5
            self.view.single_crystal_plot.set_pointsize(self._plot_param.pointsize)
        else:
            self._plot_param.lines = (self._plot_param.lines + 1) % 3
            self.view.single_crystal_plot.set_linecolor(self._plot_param.lines)
        self.view.draw()

    def _set_ax_formatter(self):
        axistype = self.view.get_axis_type()
        format_coord = self.model.get_format_coord(axistype)
        self.view.single_crystal_plot.set_format_coord(format_coord)

    def _manual_lim_changed(self):
        xlim, ylim, _dummy, _dummy = self._get_current_limits(zoom=False)
        self.view.single_crystal_plot.set_xlim(xlim)
        self.view.single_crystal_plot.set_ylim(ylim)
        self._change_cb_range(zoom=False)

    def _change_cb_range(self, zoom=True):
        xlim, ylim, zlim, _dummy = self._get_current_limits(zoom)
        self.view.single_crystal_plot.set_zlim(zlim)
        if zoom:  # saving zoom state to apply to all plots
            self._plot_param.xlim = xlim
            self._plot_param.ylim = ylim
            self._plot_param.zlim = zlim
            self._plot_param.sny_zoom_in = \
                self.view.datalist.get_checked_plots()
        if self._plot_param.projections:
            self._toggle_projections(True)
        if zoom:
            self.view.draw()

    def _home_button_clicked(self):
        self.view.single_crystal_plot.disconnect_ylim_change()

        # I have no idea why but the colorbar gets tiny if you outzoom
        # with home button in log scale, redrawing colorbar fixes the problem
        owndi = self.view.get_state()
        axistype = self.view.get_axis_type()
        if owndi['log_scale']:
            self.view.single_crystal_plot.redraw_colorbar()
        if axistype['zoom']['fix_xy']:
            dxlim, dylim = self.model.get_data_xy_lim(axistype['switch'])
            self.view.single_crystal_plot.set_xlim(dxlim)
            self.view.single_crystal_plot.set_ylim(dylim)
            self._plot_param.xlim = dxlim
            self._plot_param.ylim = dylim
            self._change_cb_range(zoom=True)
        self.view.single_crystal_plot.connect_ylim_change()

    def _get_current_xy_lim(self, zoom=False):
        owndi = self.view.get_state()
        axistype = self.view.get_axis_type()
        xlim, ylim = self.model.get_mlimits(owndi['xrange'], owndi['yrange'])
        if zoom:
            dxlim, dylim = self.view.single_crystal_plot.get_active_limits()
        else:
            dxlim, dylim = self.model.get_data_xy_lim(axistype['switch'])
        if xlim[0] is None:
            xlim = dxlim
        if ylim[0] is None:
            ylim = dylim
        if axistype['zoom']['fix_xy'] and not zoom:
            if self._plot_param.xlim[0] is not None:
                xlim = self._plot_param.xlim
            if self._plot_param.ylim[0] is not None:
                ylim = self._plot_param.ylim
        return xlim, ylim

    def _get_current_z_lim(self, xlim, ylim, zoom):
        manualz = True
        owndi = self.view.get_state()
        axistype = self.view.get_axis_type()
        if axistype['switch']:
            xlim, ylim = ylim, xlim
        dzmin, dzmax, dpzmin = self.model.get_data_zmin_max(xlim, ylim)
        zlim = self.model.get_mzlimit(owndi['zrange'])
        if zlim[0] is None:
            zlim = [dzmin, dzmax]
            manualz = False
        if owndi['log_scale'] and zlim[0] < 0:
            zlim[0] = dpzmin
        if axistype['zoom']['fix_z'] and not zoom:
            if self._plot_param.zlim[0] is not None:
                zlim = self._plot_param.zlim
        return zlim, manualz

    def _get_current_limits(self, zoom=True):
        xlim, ylim = self._get_current_xy_lim(zoom)
        zlim, manualz = self._get_current_z_lim(xlim, ylim, zoom)
        return xlim, ylim, zlim, manualz
