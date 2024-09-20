# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +


class XYPosition(object):
    X = None  # Float
    Y = None  # Float

    def __init__(self, X, Y):
        self.X = X
        self.Y = Y


class XYZPosition(object):
    X = None  # Float
    Y = None  # Float
    Z = None  # Float

    def __init__(self, X, Y, Z):
        self.X = X
        self.Y = Y
        self.Z = Z
