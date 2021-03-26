# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid workbench.
"""
Qt-based matplotlib canvas
"""
from qtpy.QtCore import Qt
from qtpy.QtGui import QPen
from matplotlib.backends.backend_qt5agg import (  # noqa: F401
    FigureCanvasQTAgg, draw_if_interactive, show)


class MantidFigureCanvas(FigureCanvasQTAgg):
    def __init__(self, figure):
        super().__init__(figure=figure)
        self._pen_color = Qt.black
        self._pen_thickness = 1

    # options controlling the pen used by tools that manipulate the graph - e.g the zoom box
    @property
    def pen_color(self):
        return self._pen_color

    @pen_color.setter
    def pen_color(self, color):
        self._pen_color = color

    @property
    def pen_thickness(self):
        return self._pen_thickness

    @pen_thickness.setter
    def pen_thickness(self, thickness):
        self._pen_thickness = thickness

    # Method used by the zoom box tool on the matplotlib toolbar
    def drawRectangle(self, rect):
        # Draw the zoom rectangle to the QPainter.  _draw_rect_callback needs
        # to be called at the end of paintEvent.
        if rect is not None:

            def _draw_rect_callback(painter):
                pen = QPen(self.pen_color, self.pen_thickness / self._dpi_ratio, Qt.DotLine)
                painter.setPen(pen)
                painter.drawRect(*(pt / self._dpi_ratio for pt in rect))
        else:

            def _draw_rect_callback(painter):
                return

        self._draw_rect_callback = _draw_rect_callback
        self.update()
