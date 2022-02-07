# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.
# std imports
from typing import Tuple, Sequence, Optional

# 3rd party
import numpy as np

from mantidqt.widgets.sliceviewer.transform import NonOrthogonalTransform, OrthogonalTransform

# Types
SlicePointType = Optional[float]
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
        axes_angles: matrix of angles between Q axes
    """

    def __init__(self,
                 *,
                 point: SlicePointType,
                 transpose: bool,
                 range: DimensionRangeCollection,
                 qflags: Sequence[bool],
                 axes_angles: Optional[np.ndarray] = None):
        assert len(point) == len(qflags)
        assert 3 >= sum(1 for i in filter(
            lambda x: x is True, qflags)), "A maximum of 3 spatial dimensions can be specified"
        self.slicepoint = point
        self.range = range
        self._slicevalue_z, self._slicewidth_z = (None,) * 2
        self._display_x, self._display_y, self._display_z = (None,) * 3

        # initialise attributes
        self._init_display_indices(transpose, qflags)
        self._init_transform(qflags, axes_angles)

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

    def get_northogonal_transform(self) -> NonOrthogonalTransform:
        return self._transform

    def transform(self, point: Sequence) -> np.ndarray:
        """Transform a point to the slice frame.
        It returns a ndarray(X,Y,Z) where X,Y are coordinates of X,Y of the display
        and Z is the out of place coordinate
        :param point: A 3D point in the slice frame
        """
        return np.array((*self._transform.tr(point[self._display_x], point[self._display_y]),
                         point[self._display_z]))

    def inverse_transform(self, point: Sequence) -> np.ndarray:
        """Does the inverse transform (inverse of self.transform) from slice
        frame to data frame

        :param point: A 3D point in the slice frame
        """
        transform = np.zeros((3, 3))
        transform[0][self._display_x] = 1
        transform[1][self._display_y] = 1
        transform[2][self._display_z] = 1
        inv_trans = np.linalg.inv(transform)
        point = np.dot(inv_trans, point)
        return np.array((*self._transform.inv_tr(point[0], point[1]), point[2]))

    # private api
    def _init_display_indices(self, transpose: bool, qflags: Sequence[bool]):
        """
        Set values of the display indexes into a point in data space for coordinates X,Y,Z in display space
        For example,  slicepoint = [None,0.5,None] and transpose=True gives

            x_index=2, y_index=0, z_index=1

        For parameter descriptions see __init__
        :return: A list of 3 indices corresponding to the display mapping
        """

        # find display indices
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

        self._display_x, self._display_y, self._display_z = x_index, y_index, z_index

        # get out of plane center and width
        if z_index is not None:
            self._slicevalue_z = self.slicepoint[z_index]
            z_range = self.range[z_index]
            self._slicewidth_z = z_range[1] - z_range[0]

    def _init_transform(self, qflags: Sequence[bool], axes_angles: Optional[np.ndarray] = None):
        # find transform for the chosen display indices
        if isinstance(axes_angles, np.ndarray) and qflags[self._display_x] and qflags[self._display_y]:
            # adjust index if non-Q dimension is before a Q the displayed axes
            # force array to have dtype=bool otherwise ~ operator throws error on empty array when index is zero
            ix = self._display_x - np.sum(~np.array(qflags[:self._display_x], dtype=bool))
            iy = self._display_y - np.sum(~np.array(qflags[:self._display_y], dtype=bool))
            angle = axes_angles[ix, iy]
            if abs(angle-(np.pi/2)) < 1e-5:
                self._transform = OrthogonalTransform()  # use OrthogonalTransform for performance
            else:
                self._transform = NonOrthogonalTransform(axes_angles[ix, iy])
            self._nonorthogonal_axes_supported = True
        else:
            self._transform = OrthogonalTransform()
            self._nonorthogonal_axes_supported = False
