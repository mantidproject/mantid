# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Tof Powder Plot widget
"""
from mantidqtinterfaces.dns.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns.plot.tof_powder_plot_model import \
    DNSTofPowderPlotModel
from mantidqtinterfaces.dns.plot.tof_powder_plot_presenter import \
    DNSTofPowderPlotPresenter
from mantidqtinterfaces.dns.plot.tof_powder_plot_view import \
    DNSTofPowderPlotView


class DNSTofPowderPlotWidget(DNSWidget):

    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSTofPowderPlotView(parent=parent.view)
        self.model = DNSTofPowderPlotModel(parent=self)
        self.presenter = DNSTofPowderPlotPresenter(parent=self,
                                                   view=self.view,
                                                   model=self.model,
                                                   name=name)
