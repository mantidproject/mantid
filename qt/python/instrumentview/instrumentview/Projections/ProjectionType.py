# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum


class ProjectionType(str, Enum):
    THREE_D = "3D"
    SPHERICAL_X = "Spherical X"
    SPHERICAL_Y = "Spherical Y"
    SPHERICAL_Z = "Spherical Z"
    CYLINDRICAL_X = "Cylindrical X"
    CYLINDRICAL_Y = "Cylindrical Y"
    CYLINDRICAL_Z = "Cylindrical Z"
    SIDE_BY_SIDE = "Side by Side"
