# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy import ndarray


class PlotPresenter:
    def __init__(self, view):
        self._view = view

    def plot(self, x_data: ndarray, y_data: ndarray, grid_lines: bool, colour_code: str) -> None:
        self._view.plot_data(x_data, y_data, grid_lines, colour_code, "x")
