# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS elastic powder plot presenter
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_observer import \
    DNSObserver


class DNSElasticPowderPlotPresenter(DNSObserver):
    def __init__(self, name=None, parent=None, view=None, model=None):
        super().__init__(parent=parent, name=name, view=view, model=model)
        self.view.sig_plot.connect(self._plot)
        self.view.sig_gridstate_change.connect(self._change_gridstate)
        self.view.sig_errorbar_change.connect(self._change_errorbar)
        self.view.sig_linestyle_change.connect(self._change_linestyle)
        self.view.sig_log_change.connect(self._change_log)

        self._errorbar = 0
        self._gridstate = 0
        self._linestyle = 0

    def _change_log(self, log):
        if log:
            self.view.set_yscale('symlog')
        else:
            self.view.set_yscale('linear')

    def _change_linestyle(self):
        self._linestyle = (self._linestyle + 1) % 3
        self._plot()

    def _change_errorbar(self):
        self._errorbar = (self._errorbar + 1) % 3
        self._plot()

    def _change_gridstate(self, draw):
        if draw:
            self._gridstate = (self._gridstate + 1) % 3
        if self._gridstate == 1:
            self.view.set_major_grid()
        elif self._gridstate == 2:
            self.view.set_major_minor_grid()
        else:
            self.view.set_no_grid()
        if draw:
            self.view.draw()

    def _plot(self):
        checked_workspaces = self.view.get_check_plots()
        wavelength = self._get_wavelength()
        norm = self._get_norm()
        xaxis = self.view.get_xaxis()
        xaxis_label = self.model.get_x_axis_label(xaxis)
        self.view.create_plot(norm=self.model.get_y_norm_label(norm))
        max_int = self.model.get_max_int_of_workspaces(checked_workspaces)
        for ws in checked_workspaces:
            x, y, yerr = self.model.get_x_y_yerr(ws, xaxis, max_int,
                                                 wavelength)
            self._single_plot(ws, x, y, yerr)
        self.view.finish_plot(xaxis_label)

    def _single_plot(self, ws, x, y, yerr):
        if self._errorbar:
            self.view.single_error_plot(x,
                                        y,
                                        yerr,
                                        label=f'{ws}'.strip(' _'),
                                        capsize=(self._errorbar - 1) * 3,
                                        linestyle=self._linestyle)
        else:
            self.view.single_plot(x,
                                  y,
                                  label=f'{ws}'.strip(' _'),
                                  linestyle=self._linestyle)

    def _auto_select_curve(self):
        if self.param_dict['elastic_powder_options']["separation"]:
            self.view.check_seperated()
        else:
            self.view.check_first()

    # short names for readability
    def _get_wavelength(self):
        return self.param_dict['elastic_powder_options']["wavelength"]

    def _get_norm(self):
        return self.param_dict['elastic_powder_options']['norm_monitor']

    def _get_workspaces(self):
        return self.param_dict['elastic_powder_script_generator']['plotlist']

    def _get_scriptnumber(self):
        return self.param_dict['elastic_powder_script_generator'][
            'script_number']

    def tab_got_focus(self):
        workspaces = self._get_workspaces()
        datalist = self.view.get_datalist()
        scriptnumber = self._get_scriptnumber()
        ws = self.model.get_updated_ws_list(workspaces, datalist, scriptnumber)
        if ws[1]:
            self.view.set_datalist([x[4:] for x in ws[0]])
            self._auto_select_curve()
            self.view.start_timer()
            # wait for plot draw, the reset to tight layout

    def process_auto_reduction_request(self):
        self.view.clear_plot()
