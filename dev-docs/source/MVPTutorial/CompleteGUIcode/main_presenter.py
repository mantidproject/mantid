# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from presenter import Presenter
from plot_presenter import PlotPresenter


class MainPresenter:

    def __init__(self, view, model):
        self._view = view
        self._model = model

        options_view = self._view.get_options_view()
        options_view.subscribe_presenter(self)

        self._presenter = Presenter(options_view, self._model.line_colours())
        self._plot_presenter = PlotPresenter(self._view.get_plot_view())

    def handle_update_plot(self) -> None:
        colour, freq, phi = self._presenter.get_plot_info()
        grid_lines = self._presenter.get_grid_lines()

        self._model.generate_y_data(freq, phi)
        x_data = self._model.get_x_data()
        y_data = self._model.get_y_data()

        self._plot_presenter.plot(x_data, y_data, grid_lines, colour)
