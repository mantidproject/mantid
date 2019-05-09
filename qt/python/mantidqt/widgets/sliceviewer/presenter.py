# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
#
#
from __future__ import (absolute_import, division, print_function)
from .model import SliceViewerModel, WS_TYPE
from .view import SliceViewerView


class SliceViewer(object):
    def __init__(self, ws, parent=None, model=None, view=None):
        # Create model and view, or accept mocked versions
        self.model = model if model else SliceViewerModel(ws)

        if self.model.get_ws_type() == WS_TYPE.MDH:
            self.new_plot = self.new_plot_MDH
        else:
            self.new_plot = self.new_plot_matrix

        self.view = view if view else SliceViewerView(self, self.model.get_dimensions_info(), parent)

        self.new_plot()

    def new_plot_MDH(self):
        self.view.plot_MDH(self.model.get_ws(), slicepoint=self.view.dimensions.get_slicepoint())

    def new_plot_matrix(self):
        self.view.plot_matrix(self.model.get_ws())

    def update_plot_data(self):
        self.view.update_plot_data(self.model.get_data(self.view.dimensions.get_slicepoint(), self.view.dimensions.transpose))
