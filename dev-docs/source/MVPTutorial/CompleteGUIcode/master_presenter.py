from __future__ import (absolute_import, division, print_function)


import presenter
import plot_presenter


class MasterPresenter(object):

    def __init__(self, view, data_model, colour_list):
        self.view = view

        self.data_model = data_model

        self.presenter = presenter.Presenter(self.view.getOptionView(), colour_list)
        self.plot_presenter = plot_presenter.PlotPresenter(self.view.getPlotView())
        # connect statements
        self.view.getOptionView().plotSignal.connect(self.updatePlot)

    # handle signals
    def updatePlot(self):
        # only care about the colour if the button is pressed
        colour, freq, phi = self.presenter.getPlotInfo()
        grid_lines = self.presenter.getGridLines()

        self.data_model.genData(freq, phi)
        x_data = self.data_model.getXData()
        y_data = self.data_model.getYData()

        self.plot_presenter.plot(x_data, y_data, grid_lines, colour)
