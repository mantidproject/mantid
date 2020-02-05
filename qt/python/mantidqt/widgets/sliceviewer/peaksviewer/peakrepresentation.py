# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import (absolute_import, division, unicode_literals)

# constants
PEAK_COLOR = 'lightgray'
PEAK_ALPHA = 0.8


class PeakRepresentation(object):
    """Describes a representation of Peak for display"""

    def __init__(self, center, alpha, color):
        """
        :param center: A V3D defining the center
        :param alpha: A float between 0.0, 1.0 defining the transparency
        :param center: A str code defining the color of the peak marker
        """
        self.center = center
        self.alpha = alpha


def create_peakrepresentation(peak):
    """
    A factory function to create an appropriate PeakRepresentation
    object for a peak.
    :param peak: A Peak object
    :returns: A PeakRepresentation object describing the Peak aspects
              important for display
    """
    return PeakRepresentation(center=peak.getQLabFrame(), alpha=PEAK_ALPHA,
                              color=PEAK_COLOR)
