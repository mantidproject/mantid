# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals


class PeakRepresentationColorSelection(object):
    """Defines behaviour for selection of colors for the
    PeakRepresentation objects"""

    def __init__(self, color, background_color):
        """
        :param color: The initial selected color
        """
        self._selected_color = color

    @property
    def marker_color(self):
        """Returns the current color selection
        :returns: str describing the color
        """
        return self._selected_color

    # @property
    # def background_color(self):
    #     """Returns the current color selection
    #     :returns: str describing the color
    #     """
    #     return self._selected_color
