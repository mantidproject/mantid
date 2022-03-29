# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
DNS Path Configuration Widget = View - Tab of DNS Reduction GUI
"""

import numpy as np
from qtpy.QtWidgets import QVBoxLayout
from mantidqt.widgets.sliceviewer.presenters.presenter import SliceViewer
from mantidqtinterfaces.dns.data_structures.dns_view import DNSView



class DNSTofPowderPlotView(DNSView):
    """
        Widget for basic plotting of DNS powder TOF data
    """
    NAME = 'Plotting'

    def __init__(self, parent):
        super().__init__(parent)
        self.layout = QVBoxLayout()
        self.setLayout(self.layout)
        self.slice_viewer = None

    def set_plot(self, workspace):
        if self.slice_viewer is None:
            np.seterr(divide='ignore', invalid='ignore')
            self.slice_viewer = SliceViewer(workspace)
            self.layout.addWidget(self.slice_viewer.view)
            np.seterr(divide='warn', invalid='warn')
        else:
            np.seterr(divide='ignore', invalid='ignore')
            self.slice_viewer.update_plot_data()
            self.slice_viewer.view.data_view.on_home_clicked()
            np.seterr(divide='warn', invalid='warn')
