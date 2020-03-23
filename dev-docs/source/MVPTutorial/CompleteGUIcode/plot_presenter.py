# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)


class PlotPresenter(object):

    def __init__(self, view):
        self.view = view

    def plot(self, x_data, y_data, grid_lines, colour_code):
        self.view.addData(x_data, y_data, grid_lines, colour_code, "x")
