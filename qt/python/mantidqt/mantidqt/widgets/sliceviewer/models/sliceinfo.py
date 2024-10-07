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

from mantidqt.widgets.sliceviewer.models.transform import NonOrthogonalTransform, OrthogonalTransform

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

    def __init__(
        self,
        *,
        point: SlicePointType,
        transpose: bool,
        range: DimensionRangeCollection,
        qflags: Sequence[bool],
        axes_angles: Optional[np.ndarray] = None,
        proj_matrix=np.eye(3),
    ):
        assert len(point) == len(qflags)
        assert 3 >= sum(1 for i in filter(lambda x: x is True, qflags)), "A maximum of 3 spatial dimensions can be specified"
        self.slicepoint = point
        self.range = range
        self._slicevalue_z, self._slicewidth_z = (None,) * 2
        self._display_x, self._display_y, self._display_z = (None,) * 3
        self.i_non_q = np.array([idim for idim, qflag in enumerate(qflags) if not qflag])
        self.proj_matrix = np.asarray(proj_matrix)

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

    def can_support_peak_overlay(self) -> bool:
        return self._peak_overlay_supported

    def get_northogonal_transform(self) -> NonOrthogonalTransform:
        return self._transform

    def transform(self, point: Sequence) -> np.ndarray:
        """Transform a 3-vector point (e.g. HLK, Qxyz) to the slice frame/display coordinates xy (z = slicepoint)
        It returns a ndarray(X,Y,Z) where X,Y are coordinates of X,Y of the display
        and Z is the out of place coordinate
        :param point: A 3D point in the slice frame
        """
        point = np.linalg.inv(self.proj_matrix).dot(point)
        px = point[self.adjust_index_for_preceding_nonq_dims(self._display_x)]
        py = point[self.adjust_index_for_preceding_nonq_dims(self._display_y)]
        pz = point[self.adjust_index_for_preceding_nonq_dims(self._display_z)]
        return np.array((*self._transform.tr(px, py), pz))

    def inverse_transform(self, point: Sequence) -> np.ndarray:
        """
        Does the inverse transform (inverse of self.transform) from slice/display frame to data frame
        :param point: A 3D point in the data frame
        """
        x, y = self._transform.inv_tr(point[0], point[1])  # apply inverse transform to get x,y in data frame
        # populate data point accounting for order of axes displayed
        data_point = np.zeros(3)
        data_point[self.adjust_index_for_preceding_nonq_dims(self._display_x)] = x
        data_point[self.adjust_index_for_preceding_nonq_dims(self._display_y)] = y
        data_point[self.adjust_index_for_preceding_nonq_dims(self._display_z)] = point[2]
        return self.proj_matrix.dot(data_point)

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
        self._peak_overlay_supported = qflags[self._display_x] and qflags[self._display_y]
        # find transform for the chosen display indices
        if isinstance(axes_angles, np.ndarray) and self._peak_overlay_supported:
            ix = self.adjust_index_for_preceding_nonq_dims(self._display_x)
            iy = self.adjust_index_for_preceding_nonq_dims(self._display_y)
            angle = axes_angles[ix, iy]
            if abs(angle - (np.pi / 2)) < 1e-5:
                self._transform = OrthogonalTransform()  # use OrthogonalTransform for performance
            else:
                self._transform = NonOrthogonalTransform(axes_angles[ix, iy])
            self._nonorthogonal_axes_supported = True
        else:
            self._transform = OrthogonalTransform()
            self._nonorthogonal_axes_supported = False

    def adjust_index_for_preceding_nonq_dims(self, display_index: int) -> int:
        """
        Get index of q dimension corresponding to display index - i.e. ignoring non q dims.
        :param display_index: index of dimension displayed
        :return: index of q dim ignoring non q dims - i.e. index in range(0,3)
        """
        return int(display_index - np.sum(self.i_non_q < display_index))  # cast to int from numpy int32 type

    def is_xy_q_frame(self) -> tuple:
        """
        Determine if the display x- and y-axis are part of a q-frame
        :return: 2-tuple of bool whether the x- and y-axis are in a q-frame
        """
        return (self._display_x not in self.i_non_q, self._display_y not in self.i_non_q)
