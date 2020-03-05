# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
from __future__ import absolute_import

import unittest

import matplotlib.pyplot as plt
from numpy import testing as np_testing

from mantid.plots.utility import zoom, zoom_axis


class TestUtility(unittest.TestCase):

    def test_zoom_axis_raises_for_invalid_axis(self):
        self.assertRaises(ValueError, zoom_axis, None, None, 'z', None)

    def test_zoom_sets_correct_axis_limits(self):
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [6, 4, 6])

        zoom_point = [2, 6]
        factor = 1/1.1  # = 0.909090...
        xlims, ylims = zoom(ax, *zoom_point, factor=factor)

        np_testing.assert_almost_equal([-0.31, 2.11], xlims)
        np_testing.assert_almost_equal([3.69, 6.11], ylims)


if __name__ == '__main__':
    unittest.main()
