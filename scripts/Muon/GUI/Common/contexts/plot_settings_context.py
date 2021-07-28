# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class PlotSettingsContext(object):
    def __init__(self):
        self._min_y_range = 2.0
        self._y_axis_margin = 20.
        self._is_tiled = False
        self._is_tiled_by = ""
        self._is_condensed = False

    def set_condensed(self, state):
        self._is_condensed = state

    @property
    def is_condensed(self):
        return self._is_condensed

    @property
    def is_tiled(self):
        return self._is_tiled

    @property
    def is_tiled_by(self):
        return self._is_tiled_by

    def set_tiled(self, state):
        self._is_tiled = state

    def set_tiled_by(self, tiled_by):
        self._is_tiled_by = tiled_by

    @property
    def min_y_range(self):
        return self._min_y_range/2.

    @property
    def y_axis_margin(self):
        # stored as a percentage, but return decimal
        return self._y_axis_margin/100.
