# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import matplotlib.image as mimage
import numpy as np

from enum import Enum

# Threshold defining whether an image is light or dark ( x < Threshold = Dark)
THRESHOLD = 100


class ImageIntensity(Enum):
    LIGHT = 1
    DARK = 2


class MantidImage(mimage.AxesImage):
    def __init__(
        self, ax, cmap=None, norm=None, interpolation=None, origin=None, extent=None, filternorm=1, filterrad=4.0, resample=False, **kwargs
    ):
        super().__init__(
            ax,
            cmap=cmap,
            norm=norm,
            interpolation=interpolation,
            origin=origin,
            extent=extent,
            filternorm=filternorm,
            filterrad=filterrad,
            resample=resample,
            **kwargs,
        )

    def calculate_greyscale_intensity(self) -> ImageIntensity:
        """
        Calculate the intensity of the image in greyscale.
        The intensity is given in the range [0, 255] where:
        - 0 is black - i.e. a dark image and
        - 255 is white, a light image.
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
