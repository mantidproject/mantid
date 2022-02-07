# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
#  This file is part of the mantid package
import unittest

import matplotlib
matplotlib.use("Agg")  # noqa
import matplotlib.pyplot as plt
from mantid.plots.legend import LegendProperties
from numpy import testing as np_testing

from mantid.plots.utility import zoom, zoom_axis, ZOOM_LIMIT
from testhelpers import assertRaisesNothing


class TestUtility(unittest.TestCase):

    def test_zoom_axis_raises_for_invalid_axis(self):
        self.assertRaises(ValueError, zoom_axis, None, None, 'z', None)

    def test_zoom_sets_correct_axis_limits(self):
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [6, 4, 6])

        zoom_point = [2, 6]
        factor = 1 / 1.1  # = 0.909090...
        xlims, ylims = zoom(ax, *zoom_point, factor=factor)

        np_testing.assert_almost_equal([-0.31, 2.11], xlims)
        np_testing.assert_almost_equal([3.69, 6.11], ylims)

    def test_from_legend_correctly_returns_properties_if_title_is_none(self):
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [6, 4, 6])
        ax.legend(labels='A', title=None)

        legend_props = LegendProperties.from_legend(ax.legend_)

        self.assertEqual(legend_props['title'], '')

    def test_zoom_out_really_far(self):
        """
        Attempt to zoom out far enough so that the new y-axis limits are beyond the maximum size of a float.
        """
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [1e10, -1e10, 1e10])

        axis_min = -1e10
        axis_max = 1e10
        ax.set_xlim([axis_min, axis_max])
        ax.set_ylim([axis_min, axis_max])

        zoom_point = [0, 0]
        zoom_factor = 1/ZOOM_LIMIT

        # Attempt to zoom to +/-10^310
        assertRaisesNothing(self, zoom, ax, *zoom_point, zoom_factor)

        # Check the axis limits haven't changed.
        new_x_min, new_x_max = ax.get_xlim()
        new_y_min, new_y_max = ax.get_ylim()
        self.assertEqual(axis_min, new_x_min)
        self.assertEqual(axis_min, new_y_min)
        self.assertEqual(axis_max, new_x_max)
        self.assertEqual(axis_max, new_y_max)

    def test_zoom_in_from_outside_zoom_limit(self):
        """
        Manually set the axis range so it is outside of the zoom out limit, and make sure it's possible to zoom in.
        """
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [1e10, -1e10, 1e10])

        # Set axis limits outside of the maximum zoom out range.
        axis_max = ZOOM_LIMIT * 2.0
        axis_min = ZOOM_LIMIT * -2.0
        ax.set_xlim([axis_min, axis_max])
        ax.set_ylim([axis_min, axis_max])

        # Attempt to zoom in.
        zoom_point = [0, 0]
        zoom_factor = 1.05
        zoom(ax, *zoom_point, zoom_factor)

        # Check that we were able to zoom in.
        x_min, x_max = ax.get_xlim()
        y_min, y_max = ax.get_ylim()
        self.assertTrue(x_min > axis_min)
        self.assertTrue(x_max < axis_max)
        self.assertTrue(y_min > axis_min)
        self.assertTrue(y_max < axis_max)

    def test_zoom_out_with_flipped_limits(self):
        """
        It is possible to set x_min > x_max, so we need to make sure we can't zoom out beyond
        max zoom in that case.
        """
        fig, ax = plt.subplots()
        ax.plot([0, 1, 2], [1e10, -1e10, 1e10])

        # Set min > max.
        axis_max = -1e10
        axis_min = 1e10

        ax.set_xlim([axis_min, axis_max])
        ax.set_ylim([axis_min, axis_max])

        zoom_point = [0, 0]
        zoom_factor = 1/ZOOM_LIMIT

        # Attempt to zoom to -/+10^310
        assertRaisesNothing(self, zoom, ax, *zoom_point, zoom_factor)

        # Check the axis limits haven't changed.
        new_x_min, new_x_max = ax.get_xlim()
        new_y_min, new_y_max = ax.get_ylim()
        self.assertEqual(axis_min, new_x_min)
        self.assertEqual(axis_min, new_y_min)
        self.assertEqual(axis_max, new_x_max)
        self.assertEqual(axis_max, new_y_max)


if __name__ == '__main__':
    unittest.main()
