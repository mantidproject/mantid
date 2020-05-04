# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantidqt.

# std imports
from collections import namedtuple

# 3rd party
import numpy as np

# Encapsulate information current slice paramters
_SliceInfoBase = namedtuple("SliceInfo", ("indices", "frame", "point", "range"))


class SliceInfo(_SliceInfoBase):
    """
    Keep information regarding the current slice of data. Allows transforming a point
    to the slice coordinate system.

    Fields:
        indices: A list of 3 indices, (a,b,c),
                 where a=index of displayed X dimension,
                 b=index of displayed Y dimension and c=index of out of plane slicing dimension
        frame: A str indicating the frame of the slice
        point: A float storing the current slice point
        range: A 2-tuple giving the range of the slice in the slicing dimension
    """

    @property
    def value(self):
        return self.point[self.indices[2]]

    @property
    def width(self):
        return self.range[1] - self.range[0]

    def transform(self, point):
        """Transform a point to the slice frame.
        It returns a ndarray(X,Y,Z) where X,Y are coordinates of X,Y of the display
        and Z is the out of place coordinate
        :param point: A 3D point in the slice frame
        """
        dimindices = self.indices
        return np.array((point[dimindices[0]], point[dimindices[1]], point[dimindices[2]]))
