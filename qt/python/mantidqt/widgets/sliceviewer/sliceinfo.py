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
from mantid.api import SpecialCoordinateSystem
import numpy as np

# Types
SlicePointType = Sequence[Union[None, float]]
DimensionRange = Tuple[float, float]
DimensionRangeCollection = Sequence[DimensionRange]


class SliceInfo:
    """
    Keep information regarding the current slice of data. Allows for transforming a point
    to the slice coordinate system.

    Fields:
        frame: A SpecialCoordinateSystem enum indicating the frame of the slice
        point: A list where None indicates a non-integrated dimension and
               a float gives the current value of a slice at this dimension
        transpose: A bool indicating if the displayed dimensions are swapped
        range: A list of 2-tuple giving the range of the slice in the slicing dimension
               or None for a non-slice dimension
        qflags: A list of booleans indicating if a dimension is a Q dimension or not.
                There can only be a maximum of 3 dimensions flagged.
    """
    def __init__(self, *, frame: SpecialCoordinateSystem, point: SlicePointType, transpose: bool,
                 range: DimensionRangeCollection, qflags: Sequence[bool]):
        assert len(point) == len(qflags)
        assert 3 >= sum(1 for i in filter(lambda x: x is True, qflags)
                        ), "A maximum of 3 spatial dimensions can be specified"
        self.frame = frame
        self.slicepoint = point
        self.range = range
        self._slicevalue_z, self._slicewidth_z = (None, ) * 2
        self._display_x, self._display_y, self._display_z = (None, ) * 3
        self._init(transpose, qflags)

    @property
    def z_index(self) -> float:
        """Return the index of the dimension treated as the out of plane index"""
        return self._display_z

    @property
    def z_value(self) -> float:
        """Return the value of the slice point in the dimension assigned as Z"""
        return self._slicevalue_z

    @property
    def z_width(self) -> float:
        """Return the width of the slice in the dimension assigned as Z. Can be None if there is no Z"""
        return self._slicewidth_z

    def can_support_nonorthogonal_axes(self) -> bool:
        """Return boolean indicating if the current slice selection can support nonorthogonal viewing.
        Both display axes must be spatial for this to be supported.
        """
        return self._nonorthogonal_axes_supported

    def transform(self, point) -> np.ndarray:
        """Transform a point to the slice frame.
        It returns a ndarray(X,Y,Z) where X,Y are coordinates of X,Y of the display
        and Z is the out of place coordinate
        :param point: A 3D point in the slice frame
        """
        return np.array((point[self._display_x], point[self._display_y], point[self._display_z]))

    # private api
    def _init(self, transpose: bool, qflags: Sequence[bool]):
        """
        Set values of the display indexes into a point in data space for coordinates X,Y,Z in display space
        For example,  slicepoint = [None,0.5,None] and transpose=True gives

            x_index=2, y_index=0, z_index=1

        For parameter descriptions see __init__
        :return: A list of 3 indices corresponding to the display mapping
        """
        x_index, y_index, z_index = None, None, None
        for index, (pt, is_spatial) in enumerate(zip(self.slicepoint, qflags)):
            if pt is None:
                if x_index is None:
                    x_index = index
                else:
                    y_index = index
            elif is_spatial:
                z_index = index

        if transpose:
            x_index, y_index = y_index, x_index

        self._nonorthogonal_axes_supported = (self.frame == SpecialCoordinateSystem.HKL
                                              and qflags[x_index] and qflags[y_index])

        if z_index is not None:
            self._slicevalue_z = self.slicepoint[z_index]
            z_range = self.range[z_index]
            self._slicewidth_z = z_range[1] - z_range[0]

        self._display_x, self._display_y, self._display_z = x_index, y_index, z_index
