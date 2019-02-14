# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import absolute_import, print_function


class Label(object):

    def __init__(
      self,
      text,
      xvalue,
      xrelative,
      yvalue,
      yrelative,
      rotation=0,
      protected=False):
        self._text = text
        self._xvalue = xvalue
        self._xrelative = xrelative
        self._yvalue = yvalue
        self._yrelative = yrelative
        self._rotation = rotation
        self._protected = protected

    @property
    def text(self):
        return self._text

    @property
    def rotation(self):
        return self._rotation

    @property
    def protected(self):
        return self._protected

    def get_xval(self, x_range):
        if self._xrelative:
            return self._xvalue
        else:
            return self.relative(self._xvalue, x_range)

    def get_yval(self, y_range):
        if self._yrelative:
            return self._yvalue
        else:
            return self.relative(self._yvalue, y_range)

    def in_x_range(self, x_range):
        if self._xvalue < x_range[0] or self._xvalue > x_range[1]:
            return False
        return True

    def relative(self, value, value_range):
        if value < value_range[0] or value > value_range[1]:
            return None
        return (value - value_range[0]) / (value_range[1] - value_range[0])
