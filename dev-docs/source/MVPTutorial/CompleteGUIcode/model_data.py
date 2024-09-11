# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from numpy import linspace, ndarray, sin
from typing import List


class PlotModel:

    def __init__(self):
        self._x_data = linspace(0.0, 10.0, 100)
        self._y_data = []

    def generate_y_data(self, freq: float, phi: float) -> None:
        self._y_data = sin(freq * self._x_data + phi)

    def get_x_data(self) -> ndarray:
        return self._x_data

    def get_y_data(self) -> ndarray:
        return self._y_data

    @staticmethod
    def line_colours() -> List[str]:
        return ["red", "blue", "black"]
