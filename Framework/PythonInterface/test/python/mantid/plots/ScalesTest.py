# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest

from matplotlib.scale import scale_factory
from mantid.plots.scales import PowerScale, SquareScale
import numpy as np
import testhelpers

class ScalesTest(unittest.TestCase):

    def test_power_scale_registered_in_factory(self):
        self.assertTrue(isinstance(scale_factory('power', axis=None, gamma=3),
                                   PowerScale))

    def test_power_transform(self):
        gamma = 3
        scale = PowerScale(None, gamma=gamma)
        x = np.linspace(0, 10, 1)
        transform = scale.get_transform()
        testhelpers.assert_almost_equal(np.power(x, gamma),
                                       transform.transform_non_affine(x))

    def test_power_inverse_transform(self):
        gamma = 3
        scale = PowerScale(None, gamma=gamma)
        x = np.linspace(0, 10, 1)
        inv_transform = scale.get_transform().inverted()
        testhelpers.assert_almost_equal(np.power(x, 1./gamma),
                                       inv_transform.transform_non_affine(x))


    def test_square_scale_registered_in_factory(self):
        self.assertTrue(isinstance(scale_factory('square', axis=None),
                                   SquareScale))

    def test_square_transform(self):
        scale = SquareScale(None)
        x = np.linspace(0, 10, 1)
        transform = scale.get_transform()
        testhelpers.assert_almost_equal(np.power(x, 2),
                                       transform.transform_non_affine(x))


    def test_square_inverse_transform(self):
        scale = SquareScale(None)
        x = np.linspace(0, 10, 1)
        inv_transform = scale.get_transform().inverted()
        testhelpers.assert_almost_equal(np.sqrt(x),
                                       inv_transform.transform_non_affine(x))


if __name__ == '__main__':
    unittest.main()
