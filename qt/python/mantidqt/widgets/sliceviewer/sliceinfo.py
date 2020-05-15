# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
from typing import Tuple, Sequence, Union

# 3rd party
import numpy as np

# Types
SlicePointType = Sequence[Union[None, float]]
SliceRange = Tuple[float, float]


class SliceInfo:
    """
    Keep information regarding the current slice of data. Allows for transforming a point
    to the slice coordinate system.

    Fields:
        frame: A str indicating the frame of the slice
        point: A list where None indicates a non-integrated dimension and
               a float gives the current value of a slice at this dimension
        transpose: A bool indicating if the displayed dimensions are swapped
        range: A 2-tuple giving the range of the slice in the slicing dimension
    """
    def __init__(self, *, frame: str, point: SlicePointType, transpose: bool, range: SliceRange):
        assert 3 == len(
            point), f"Expected a length 3 array for slice point, found length={len(point)}"
        self.frame = frame
        self.slicepoint = point
        self.range = range
        self._display_x, self._display_y, self._display_z = self._create_display_mapping(transpose)

    @property
    def value(self):
        return self.slicepoint[self._display_z]

    @property
    def width(self):
        return self.range[1] - self.range[0]

    def transform(self, point):
        """Transform a point to the slice frame.
        It returns a ndarray(X,Y,Z) where X,Y are coordinates of X,Y of the display
        and Z is the out of place coordinate
        :param point: A 3D point in the slice frame
        """
        return np.array((point[self._display_x], point[self._display_y], point[self._display_z]))

    # private api
    def _create_display_mapping(self, transpose):
        """
        Set values of the display indexes into a point in data space for coordinates X,Y,Z in display space
        For example,  slicepoint = [None,0.5,None] and transpose=True gives

            x_index=2, y_index=0, z_index=1
        :param transpose: If True then X,Y dimensions should be swapped
        :return: A list of 3 indices corresponding to the display mapping
        """
        x_index, y_index, z_index = None, None, None
        for index, pt in enumerate(self.slicepoint):
            if pt is None:
                if x_index is None:
                    x_index = index
                else:
                    y_index = index
            else:
                z_index = index

        if transpose:
            x_index, y_index = y_index, x_index

        return x_index, y_index, z_index
