# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#    This file is part of the mantidqt package.
from __future__ import absolute_import


class ErrorColumn:
    CANNOT_SET_Y_TO_BE_OWN_YERR_MESSAGE = "Cannot set Y column to be its own YErr"
    UNHANDLED_COMPARISON_LOGIC_MESSAGE = "Unhandled comparison logic with type {}"

    def __init__(self, column, related_y_column, label_index):
        self.column = column
        self.related_y_column = related_y_column
        if self.column == self.related_y_column:
            raise ValueError(self.CANNOT_SET_Y_TO_BE_OWN_YERR_MESSAGE)

        self.label_index = label_index

    def __eq__(self, other):
        if isinstance(other, ErrorColumn):
            return self.related_y_column == other.related_y_column or self.column == other.column
        elif isinstance(other, int):
            return self.column == other
        else:
            raise RuntimeError(self.UNHANDLED_COMPARISON_LOGIC_MESSAGE.format(type(other)))

    def __cmp__(self, other):
        if isinstance(other, ErrorColumn):
            return self.column == other.column or self.related_y_column == other.related_y_column
        elif isinstance(other, int):
            return self.column == other
        else:
            raise RuntimeError(self.UNHANDLED_COMPARISON_LOGIC_MESSAGE.format(type(other)))
