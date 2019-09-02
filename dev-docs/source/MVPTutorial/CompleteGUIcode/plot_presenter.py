from __future__ import (absolute_import, division, print_function)


class PlotPresenter(object):

    def __init__(self, view):
        self.view = view

    def plot(self, x_data, y_data, grid_lines, colour_code):
        self.view.addData(x_data, y_data, grid_lines, colour_code, "x")
