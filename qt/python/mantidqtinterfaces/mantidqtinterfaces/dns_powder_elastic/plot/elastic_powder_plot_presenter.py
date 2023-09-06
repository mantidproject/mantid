# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic plotting tab presenter.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import DNSObserver


class DNSElasticPowderPlotPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.view.sig_plot.connect(self._plot)
        self.view.sig_grid_state_change.connect(self._change_grid_state)
        self.view.sig_error_bar_change.connect(self._change_error_bar)
        self.view.sig_linestyle_change.connect(self._change_line_style)
        self.view.sig_log_change.connect(self._change_log)

        self._error_bar = 2
        self._grid_state = 0
        self._line_style = 2

    def _change_log(self, log):
        if log:
            self.view.set_y_scale("symlog")
        else:
            self.view.set_y_scale("linear")

    def _change_line_style(self):
        self._line_style = (self._line_style + 1) % 3
        self._plot()

    def _change_error_bar(self):
        self._error_bar = (self._error_bar + 1) % 3
        self._plot()

    def _change_grid_state(self, draw):
        if draw:
            self._grid_state = (self._grid_state + 1) % 3
        if self._grid_state == 1:
            self.view.set_major_grid()
        elif self._grid_state == 2:
            self.view.set_major_minor_grid()
        else:
            self.view.set_no_grid()
        if draw:
            self.view.draw()

    def _plot(self):
        checked_workspaces = self.view.get_check_plots()
        wavelength = self._get_wavelength()
        norm = self._get_norm()
        x_axis = self.view.get_x_axis()
        x_axis_label = self.model.get_x_axis_label(x_axis)
        self.view.create_plot(norm=self.model.get_y_norm_label(norm))
        max_int = self.model.get_max_int_of_workspaces(checked_workspaces)
        for ws in checked_workspaces:
            x, y, y_err = self.model.get_x_y_yerr(ws, x_axis, max_int, wavelength)
            self._single_plot(ws, x, y, y_err)
        self.view.finish_plot(x_axis_label)

    def _single_plot(self, ws, x, y, y_err):
        if self._error_bar:
            self.view.single_error_plot(
                x, y, y_err, label=f"{ws}".strip(" _"), capsize=(self._error_bar - 1) * 3, linestyle=self._line_style
            )
        else:
            self.view.single_plot(x, y, label=f"{ws}".strip(" _"), linestyle=self._line_style)

    def _auto_select_curve(self):
        if self.param_dict["elastic_powder_options"]["separation"]:
            self.view.check_separated()
        else:
            self.view.check_first()

    # short names for readability
    def _get_wavelength(self):
        return self.param_dict["elastic_powder_options"]["wavelength"]

    def _get_norm(self):
        return self.param_dict["elastic_powder_options"]["norm_monitor"]

    def _get_workspaces(self):
        return self.param_dict["elastic_powder_script_generator"]["subtract"]

    def _get_script_number(self):
        return self.param_dict["elastic_powder_script_generator"]["script_number"]

    def tab_got_focus(self):
        workspaces = self._get_workspaces()
        data_list = self.view.get_data_list()
        script_number = self._get_script_number()
        ws = self.model.get_updated_ws_list(workspaces, data_list, script_number)
        if ws[1]:
            self.view.set_data_list([x[4:] for x in ws[0]])
            self._auto_select_curve()
            self.view.start_timer()
            # wait for plot draw, the reset to tight layout

    def process_auto_reduction_request(self):
        self.view.clear_plot()
