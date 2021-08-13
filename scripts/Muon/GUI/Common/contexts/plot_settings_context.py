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
        self._x_axis_margin = 10.0
        self._is_tiled = False
        self._is_tiled_by = ""
        self._is_condensed = False
        self._linestyle = {}
        self._default_linestyle = "-"
        self._marker = {}
        self._default_marker = ""
        self._wrap_width = 30# determines the width of text on axis ticks
        self._font_size = "xx-small"
        self._rotation = 45 # degrees

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

    @property
    def font_size(self):
        return self._font_size

    @property
    def wrap_width(self):
        return self._wrap_width

    @property
    def rotation(self):
        return self._rotation

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

    @property
    def x_axis_margin(self):
        # stored as a percentage, but return decimal
        return self._x_axis_margin/100.

    def set_linestyle(self, name, linestyle):
        self._linestyle[name] = linestyle

    def get_linestyle(self, name):
        if name in self._linestyle.keys():
            return self._linestyle[name]
        else:
            return self._default_linestyle

    def set_marker(self, name, marker):
        self._marker[name] = marker

    def get_marker(self, name):
        if name in self._marker.keys():
            return self._marker[name]
        else:
            return self._default_marker
