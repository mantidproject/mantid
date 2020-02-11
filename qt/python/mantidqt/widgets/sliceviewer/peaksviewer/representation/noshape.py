# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
from __future__ import absolute_import, division, unicode_literals

# std imports

# 3rdparty imports

# local imports
from .base import PEAK_ALPHA, PeakRepresentation


class PeakRepresentationNoShape(PeakRepresentation):
    """Describes representation of Peak with no shape for display"""
    MARKER = 'x'
    MARKER_SIZE_PTS_SQ = 150

    # reference to the painted object
    _painted = None

    @classmethod
    def create(cls, center, slicepoint, slicerange, _, marker_color):
        """
        Create an instance of PeakRepresentationSphere from the given set of attributes
        :param center: A V3D giving the center of the Peak
        :param slicepoint: float giving current slice point
        :param slicerange: 2-tuple giving (min/max) values of the slicing dimension
        :param _: A PeakShape object describing the shape properties (unused)
        :param marker_color: A string indicating the color of the marker
        :return: A new instance of this class
        """
        alpha = PeakRepresentation.compute_alpha(center, slicepoint, slicerange)
        return PeakRepresentationNoShape(center=center, alpha=alpha, marker_color=marker_color)

    def draw(self, painter):
        """
        Override base method and draws a single peak center marker
        :param painter: A Painter object to accept the draw command
        :return: The objects added
        """
        if self.alpha > 0.0:
            center = self.center
            self._painted = painter.scatter(
                center.X(),
                center.Y(),
                alpha=self.alpha,
                color=self.marker_color,
                marker=self.MARKER,
                s=self.MARKER_SIZE_PTS_SQ)
        else:
            self._painted = None

    def remove(self, painter):
        """Override base.
        :param painter: A Painter object to interact with the draw destination
        """
        if self._painted is not None:
            painter.remove(self._painted)

    def repaint(self, painter):
        """Override base
        :param painter: A painter to update drawn properties
        """
        # 4 "transitions" possible:
        #  - visible->visible
        #  - visible->invisible
        #  - invisible->invisible
        #  - invisible->isible
        painted = self._painted
        if self.alpha > 0.0:
            # Peak should now be visible
            if painted is not None:
                painter.update_properties(painted, alpha=self.alpha)
            else:
                # it was not drawn previously
                self.draw(painter)
        else:
            # peak becomes invisible
            if painted is not None:
                painter.remove(painted)
                self._painted = None

    def update_alpha(self, slicepoint, slicerange):
        """Update transparency value based on slicepoint
        :param painter: A Painter object to interact with the draw destination
        :param slicepoint: float giving current slice point
        :param slicerange: 2-tuple giving (min/max) values of the slicing dimension
        """
        self._alpha = PeakRepresentation.compute_alpha(self.center, slicepoint, slicerange)
