# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,

#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS elastic powder plot presenter
"""
from __future__ import (absolute_import, division, print_function)
import numpy as np

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.plot.elastic_powder_plot_view import DNSElasticPowderPlot_view

from mantid.simpleapi import mtd


class DNSElasticPowderPlot_presenter(DNSObserver):
    name = 'plot_tof_powder'

    def __init__(self, parent):
        super(DNSElasticPowderPlot_presenter,
              self).__init__(parent, 'standard_data')
        self.name = 'plot_elastic_powder'
        self.view = DNSElasticPowderPlot_view(self.parent.view)
        self.view.sig_plot.connect(self.plot)
        self.plotted_script_number = 0
        self.script_plotted = False

    def plot(self, checked_workspaces):
        if self.param_dict['elastic_powder_options']['norm_monitor']:
            norm = 'normed to monitor'
        else:
            norm = 'Counts/s'
        self.view.create_plot(norm=norm)
        xaxis = self.view.get_xaxis()
        wavelength = self.param_dict['elastic_powder_options']["wavelength"]
        max_int = 0
        for ws in checked_workspaces:
            if ws != 'simulation':
                maxy = max(mtd['mat_{}'.format(ws)].extractY()[0])
                if maxy > max_int:
                    max_int = maxy
        for ws in checked_workspaces:
            x = mtd['mat_{}'.format(ws)].extractX()[0] * 2
            x = (abs(x[1] - x[0])/2 + x)[0:-1]
            y = mtd['mat_{}'.format(ws)].extractY()[0]
            yerr = mtd['mat_{}'.format(ws)].extractE()[0]
            if ws == 'simulation':
                x = x / 2.0
                y = y / max(y) * max_int
            if xaxis == 'd':
                x = wavelength / (2 * np.sin(np.deg2rad(x / 2.0)))
            elif xaxis == 'q':
                x = np.pi * 4 * np.sin(np.deg2rad(x / 2.0)) / wavelength
            self.view.single_plot(x,
                                  y,
                                  yerr,
                                  label='{}'.format(ws).strip(' _'))
        self.view.finish_plot(xaxis)

    def tab_got_focus(self):
#        workspaces = [
#            workspace for workspace in mtd.getObjectNames()
#            if (mtd[workspace].id() == 'Workspace2D'
#                and workspace.startswith('mat_'))
#        ]
        workspaces = sorted(self.param_dict['elastic_powder'\
                                    '_script_generator']['plotlist'])
        compare = ['mat_{}'.format(x) for x in self.view.get_datalist()]
        if (self.param_dict['elastic_powder_script_generator']['script_number']
                != self.plotted_script_number
                or workspaces != compare):
            self.view.set_datalist([x[4:] for x in workspaces])
            if self.param_dict['elastic_powder_options']["separation"]:
                self.view.check_seperated()
            else:
                self.view.check_first()
            self.plotted_script_number = self.param_dict[
                'elastic_powder_script_generator']['script_number']

    def process_auto_reduction_request(self):
        self.view.clear_plot()
