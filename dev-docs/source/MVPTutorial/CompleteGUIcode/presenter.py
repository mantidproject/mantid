# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import List, Tuple


class Presenter:

    def __init__(self, view, colours: List[str]):
        self._view = view
        self._view.set_colours(colours)

    def get_plot_info(self) -> Tuple[str, float, float]:
        return str(self._view.get_colour()), self._view.get_freq(), self._view.get_phase()

    def get_grid_lines(self) -> bool:
        return self._view.get_grid_lines()
