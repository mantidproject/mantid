# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS powder tof plot presenter
"""
from __future__ import (absolute_import, division, print_function)

from DNSReduction.data_structures.dns_observer import DNSObserver
from DNSReduction.plot.tof_powder_plot_view import DNSTofPowderPlot_view

from mantid.simpleapi import mtd


class DNSTofPowderPlot_presenter(DNSObserver):
    name = 'plot_tof_powder'

    def __init__(self, parent):
        super(DNSTofPowderPlot_presenter,
              self).__init__(parent, 'standard_data')
        self.name = 'plot_tof_powder'
        self.view = DNSTofPowderPlot_view(self.parent.view)
        self.plotted_script_number = 0
        self.script_plotted = False

    def set_view_from_param(self):
        pass

    def plot(self):
        try:
            if mtd['data1_sqw'].id() == 'WorkspaceGroup':
                ## I have no idear why sometimes you get a group
                self.view.set_plot(mtd['data1_sqw'].getItem(0))
            else:
                self.view.set_plot(mtd['data1_sqw'])
            self.script_plotted = True
        except KeyError:
            self.raise_error('No processed data found, generate script first.')

    def tab_got_focus(self):
        if self.param_dict['tof_powder_script_generator'][
                'script_number'] != self.plotted_script_number:
            self.plot()
            self.plotted_script_number = self.param_dict[
                'tof_powder_script_generator']['script_number']

    def process_auto_reduction_request(self):
        self.view.clear_plot()
