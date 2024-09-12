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
from qtpy.QtGui import QPen, QColor, QPaintEvent
from matplotlib.backends.backend_qt5agg import FigureCanvasQTAgg, draw_if_interactive, show  # noqa: F401
from mantid.plots.mantidimage import MantidImage, ImageIntensity


class MantidFigureCanvas(FigureCanvasQTAgg):
    def __init__(self, figure):
        super().__init__(figure=figure)
        self._pen_color = Qt.black
        self._pen_thickness = 1.0

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

    def enterEvent(self, event):
        """
        Override of matplotlib function in backend_qt. With qt >= 5.15.6, sometimes a QPaintEvent is passed to this
        function, which will cause an attribute error. It only seems to happen when the fit browser is open in a plot
        window and the Window is resized and moved.
        """
        if not isinstance(event, QPaintEvent):
            super(MantidFigureCanvas, self).enterEvent(event)

    # Method used by the zoom box tool on the matplotlib toolbar
    def drawRectangle(self, rect):
        """Override of matplotlib.lib.matplotlib.backends.backend_qt.drawRectangle
        only to update the zoombox pen color.
        """
        self.update_pen_color()
        # Draw the zoom rectangle to the QPainter.  _draw_rect_callback needs
        # to be called at the end of paintEvent.
        if rect is not None:
            x0, y0, w, h = [int(pt / self.device_pixel_ratio) for pt in rect]
            x1 = x0 + w
            y1 = y0 + h

            def _draw_rect_callback(painter):
                pen = QPen(
                    self.pen_color,
                    self.pen_thickness / self.device_pixel_ratio,
                )
                pen.setDashPattern([3, 3])
                for color, offset in [
                    (QColor("black"), 0),
                    (QColor("white"), 3),
                ]:
                    pen.setDashOffset(offset)
                    pen.setColor(color)
                    painter.setPen(pen)
                    # Draw the lines from x0, y0 towards x1, y1 so that the
                    # dashes don't "jump" when moving the zoom box.
                    painter.drawLine(x0, y0, x0, y1)
                    painter.drawLine(x0, y0, x1, y0)
                    painter.drawLine(x0, y1, x1, y1)
                    painter.drawLine(x1, y0, x1, y1)

        else:

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
                if not isinstance(img, MantidImage):
                    continue
                intensity = img.calculate_greyscale_intensity()
                if intensity == ImageIntensity.DARK:
                    color = Qt.white
                else:
                    color = Qt.black
                self.pen_color = color
                # break after we find the first MantidImage
                break
