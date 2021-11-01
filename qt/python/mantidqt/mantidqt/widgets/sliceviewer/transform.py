# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.

# thirdparty
import numpy as np


class NonOrthogonalTransform:
    """
    Defines transformations to move between an orthogonal system and a non-orthogonal system
    defined by the lattice and projection vectors.
    """
    def __init__(self, angle: float):
        """
        :param angle: Angle in radians of skewed. Angle increases anti-clockwise from X
        """
        self._angle = angle

    @property
    def angle(self):
        return self._angle

    def tr(self, x, y):
        """Transform coordinate arrays from the non-orthogonal frame
            aligned with the crystal to the orthogonal display frame
        :param x: Numpy array of X coordinates in non-orthogonal frame
        :param y: Numpy array of Y coordinates in non-orthogonal frame
        :return: (x, y) of transformed coordinates. lengths of arrays match input
        """
        angle = self._angle
        return x + np.cos(angle) * y, np.sin(angle) * y

    def inv_tr(self, x, y):
        """Transform coordinate arrays from the orthogonal display frame
        to the non-orthogonal frame aligned with the crystal
        :param x: Numpy array of X coordinates in orthogonal display frame
        :param y: Numpy array of Y coordinates in orthogonal display frame
        :return: (x, y) of transformed coordinates. lengths of arrays match input
        """
        angle = self._angle
        return x - y / np.tan(angle), y / np.sin(angle)


class OrthogonalTransform():
    def __init__(self):
        self._angle = np.pi/2  # 90 degrees

    def tr(self, x, y):
        return x, y

    def inv_tr(self, x, y):
        return x, y
