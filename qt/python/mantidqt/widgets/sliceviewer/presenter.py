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
            self.update_plot_data = self.update_plot_data_MDH
        elif self.model.get_ws_type() == WS_TYPE.MDE:
            self.new_plot = self.new_plot_MDE
            self.update_plot_data = self.update_plot_data_MDE
        else:
            self.new_plot = self.new_plot_matrix
            self.update_plot_data = self.update_plot_data_matrix

        self.view = view if view else SliceViewerView(self, self.model.get_dimensions_info(), parent)

        self.new_plot()

    def new_plot_MDH(self):
        self.view.plot_MDH(self.model.get_ws(), slicepoint=self.view.dimensions.get_slicepoint())

    def new_plot_MDE(self):
        self.view.plot_MDH(self.model.get_ws(slicepoint=self.view.dimensions.get_slicepoint(),
                                             bin_params=self.view.dimensions.get_bin_params()))

    def new_plot_matrix(self):
        self.view.plot_matrix(self.model.get_ws())

    def update_plot_data_MDH(self):
        self.view.update_plot_data(self.model.get_data(self.view.dimensions.get_slicepoint(), self.view.dimensions.transpose))

    def update_plot_data_MDE(self):
        self.view.update_plot_data(self.model.get_data(slicepoint=self.view.dimensions.get_slicepoint(),
                                                       bin_params=self.view.dimensions.get_bin_params(),
                                                       transpose=self.view.dimensions.transpose))

    def update_plot_data_matrix(self):
        # should never be called, since this workspace type is only 2D the plot dimensions never change
        pass

    def line_plots(self):
        self.view.create_axes()
        self.new_plot()
