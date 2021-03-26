# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.image as mimage
import numpy as np
from qtpy.QtCore import Qt

from enum import Enum

# Threshold defining whether an image is light or dark ( x < Threshold = Dark)
THRESHOLD = 100


class ImageIntensity(Enum):
    LIGHT = 1
    DARK = 2


class MantidImage(mimage.AxesImage):
    def __init__(self,
                 ax,
                 cmap=None,
                 norm=None,
                 interpolation=None,
                 origin=None,
                 extent=None,
                 filternorm=1,
                 filterrad=4.0,
                 resample=False,
                 **kwargs):
        super().__init__(ax,
                         cmap=cmap,
                         norm=norm,
                         interpolation=interpolation,
                         origin=origin,
                         extent=extent,
                         filternorm=filternorm,
                         filterrad=filterrad,
                         resample=resample,
                         **kwargs)

        # Increase the default pen thickness
        self.update_pen_thickness(1.5)

    def draw(self, renderer, *args, **kwargs):
        self.update_pen_color()
        super().draw(renderer, *args, **kwargs)

    def update_pen_color(self, color=None):
        """Update the pen color used to draw tool in the matplotlib toolbar, e.g
        the zoombox. If no color is specified the color is automatically determined
        by considering how dark, or light the image is and setting a pen appropriately.
        If the canvas is not a MantidFigureCanvas, the method will be skipped.
        :param color: A qt color instance
        """
        from workbench.plotting.mantidfigurecanvas import MantidFigureCanvas
        if not isinstance(self.axes.get_figure().canvas, MantidFigureCanvas):
            return
        if color is None:
            image_intensity = self._calculate_greyscale_intensity()
            if image_intensity == ImageIntensity.DARK:
                color = Qt.white
            else:
                color = Qt.black
        self.axes.get_figure().canvas.pen_color = color

    def update_pen_thickness(self, value):
        """Update the pen thickness used to draw tool in the matplotlib toolbar, e.g
        the zoombox.
        If the canvas is not a MantidFigureCanvas, the method will be skipped.
        :param value: Thickness of the pen line
        """
        from workbench.plotting.mantidfigurecanvas import MantidFigureCanvas
        if isinstance(self.axes.get_figure().canvas, MantidFigureCanvas):
            self.axes.get_figure().canvas.pen_thickness = value

    def _calculate_greyscale_intensity(self) -> ImageIntensity:
        """
        Calculate the intensity of the image in greyscale.
        The intensity is given in the range [0, 255] where:
        -0 is black - i.e. a dark image and
        -255 is white, a light image.
        """
        rgb = self.to_rgba(self._A, alpha=None, bytes=True, norm=True)
        r, g, b = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]
        # CCIR 601 conversion from rgb to luma/greyscale
        # see https://en.wikipedia.org/wiki/Luma_(video)
        grey = 0.2989 * r + 0.5870 * g + 0.1140 * b
        mean = np.mean(grey)
        if mean > THRESHOLD:
            return ImageIntensity.LIGHT
        else:
            return ImageIntensity.DARK
