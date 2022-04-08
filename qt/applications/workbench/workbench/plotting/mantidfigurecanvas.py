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
from distutils.version import LooseVersion
from qtpy.QtCore import Qt
from qtpy.QtGui import QPen
import matplotlib
from matplotlib.backends.backend_qt5agg import (  # noqa: F401
    FigureCanvasQTAgg, draw_if_interactive, show)
from mantid.plots.mantidimage import MantidImage, ImageIntensity


class MantidFigureCanvas(FigureCanvasQTAgg):
    def __init__(self, figure):
        super().__init__(figure=figure)
        self._pen_color = Qt.black
        self._pen_thickness = 1.5
        if LooseVersion(matplotlib.__version__) >= LooseVersion("3.5.0"):
            self._dpi_ratio = self.devicePixelRatio() or 1

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
        self.update_pen_color()
        # Draw the zoom rectangle to the QPainter.  _draw_rect_callback needs
        # to be called at the end of paintEvent.
        if rect is not None:
            self._drawRect = [pt / self._dpi_ratio for pt in rect]

            def _draw_rect_callback(painter):
                pen = QPen(self.pen_color, self.pen_thickness / self._dpi_ratio, Qt.DotLine)
                painter.setPen(pen)
                painter.drawRect(*(pt / self._dpi_ratio for pt in rect))
        else:
            self._drawRect = None

            def _draw_rect_callback(painter):
                return

        self._draw_rect_callback = _draw_rect_callback
        self.update()

    def update_pen_color(self):
        """Update the pen color used to draw tool in the matplotlib toolbar, e.g
        the zoombox. The color is automatically determined
        by considering how dark, or light the image is and setting a pen appropriately.
        Only works if the figure contains a MantidImage.
        """
        for ax in self.figure.get_axes():
            for img in ax.get_images():
                if (not isinstance(img, MantidImage)):
                    continue
                intensity = img.calculate_greyscale_intensity()
                if intensity == ImageIntensity.DARK:
                    color = Qt.white
                else:
                    color = Qt.black
                self.pen_color = color
                # break after we find the first MantidImage
                break
