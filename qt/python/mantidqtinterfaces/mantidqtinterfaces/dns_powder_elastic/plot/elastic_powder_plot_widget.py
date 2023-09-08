# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +

"""
DNS powder elastic plotting widget.
"""

from mantidqtinterfaces.dns_powder_tof.data_structures.dns_widget import DNSWidget
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_model import DNSElasticPowderPlotModel
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_presenter import DNSElasticPowderPlotPresenter
from mantidqtinterfaces.dns_powder_elastic.plot.elastic_powder_plot_view import DNSElasticPowderPlotView


class DNSElasticPowderPlotWidget(DNSWidget):
    def __init__(self, name, parent):
        super().__init__(name, parent)
        self.view = DNSElasticPowderPlotView(parent=parent.view)
        self.model = DNSElasticPowderPlotModel(parent=self)
        self.presenter = DNSElasticPowderPlotPresenter(parent=self, view=self.view, model=self.model, name=name)
