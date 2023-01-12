# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import math
import numpy as np


from ansto_common import (
    Point,
    triangle_base_area,
    fractional_facet_area,
    find_intercept,
    fractional_quad_area,
    fractional_map,
    extrapolate_grid_edges,
)


def rotate(p, deg):
    theta = math.pi * deg / 180.0
    x = p.x * math.cos(theta) - p.y * math.sin(theta)
    y = p.y * math.cos(theta) + p.x * math.sin(theta)
    return Point(x, y, p.z)


class FractionalMapTests(unittest.TestCase):
    def setUp(self):
        return

    def tearDown(self):
        return

    def test_find_intercept(self):

        data = [
            (Point(0, 0, 0), Point(0, 0, 10), 0, Point(0, 0, 0)),
            (Point(0, 0, 0), Point(0, 0, 10), 5, Point(0, 0, 5)),
            (Point(0, 0, 0), Point(0, 10, 10), 5, Point(0, 5, 5)),
            (Point(0, 0, 0), Point(10, 10, 10), 5, Point(5, 5, 5)),
            (Point(0, 0, 0), Point(10, 10, 10), -5, Point(-5, -5, -5)),
        ]

        for p1, p2, qlevel, pt in data:
            p = find_intercept(p1, p2, qlevel)
            self.assertAlmostEqual(p.x, pt.x, 7)
            self.assertAlmostEqual(p.y, pt.y, 7)
            self.assertAlmostEqual(p.z, pt.z, 7)

    def test_triangle_base_area(self):
        def rotate(p, deg):
            theta = math.pi * deg / 180.0
            x = p.x * math.cos(theta) - p.y * math.sin(theta)
            y = p.y * math.cos(theta) + p.x * math.sin(theta)
            return Point(x, y, p.z)

        base = (Point(1, 1, 0), Point(3, 1, 0), Point(3, 2, 0))  # area = 1
        area = triangle_base_area(*base)
        self.assertAlmostEqual(area, 1.0, 7)

        data = []
        for deg in [0, 22.5, 45, 90, 180, 225]:
            pv = tuple([rotate(p, deg) for p in base])
            data.append((pv, 1.0))

        for args, xval in data:
            xret = triangle_base_area(*args)
            self.assertAlmostEqual(xret, xval, 7)

    def test_fractional_facet_area(self):
        # confirm the area in the region defined by the interecpt points that
        # lie on the unit square and the minimum point on the square
        data = [
            (Point(0, 0, 0), Point(1, 0, 0), Point(0.5, 0.5, 0), -1, 0),
            (Point(0, 0, 0), Point(1, 0, 0), Point(0.5, 0.5, 1), 0, 0),
            (Point(0, 0, 0), Point(1, 0, 0), Point(0.5, 0.5, 0), 1, 0.25),
            (Point(0, 0, 0), Point(1, 0, 1), Point(0.5, 0.5, 2), 1, 0.125),
            (Point(0, 0, 0), Point(1, 0, 0), Point(0.5, 0.5, 2), 1, 0.1875),
            (Point(0, 0, 2), Point(1, 0, 2), Point(0.5, 0.5, 0), 1, 0.0625),
            (
                rotate(Point(0, 0, 2), 90),
                rotate(Point(1, 0, 2), 90),
                rotate(Point(0.5, 0.5, 0), 90),
                1,
                0.0625,
            ),
        ]

        for p1, p2, p3, qlevel, area in data:
            retn = fractional_facet_area(p1, p2, p3, qlevel)
            self.assertAlmostEqual(retn, area, 7)

    def test_fractional_quad_area(self):
        data = [
            ((1, 1, 3, 3, 2), 2, 0.5),
            ((3, 1, 1, 3, 2), 2, 0.5),
            ((1, 1, 1, 1, 3), 2, 0.75),
            ((1, 1, 1, 1, -1), 0, 0.25),
            ((1, 3, 5, 3, 3), 3, 0.5),
            ((1, 3, 1, 3, 3), 3, 1.0),
            ((1, 2, 4, 3, 2.5), 3, 0.75),
            ((1, 2, 4, 3, 2.5), 2, 0.25),
            ((1, 2, 1, 0, 2), 1, 0.25),
            ((1, 2, 1, 0, 0), 1, 0.75),
            ((-1.5, -0.5, 1.5, 0.5, 0), 0, 0.5),
            ((-0.5, 0.5, 2.5, 1.5, 1), 0, 0.0625),
        ]
        for zvals, level, area in data:
            retn = fractional_quad_area(zvals, level)
            self.assertAlmostEqual(retn, area, 7)

    def test_extrapolate_grid_edges(self):
        # for a linear function the extrapolation should be exact
        rows, cols = 11, 11
        x = np.linspace(0, cols - 1, cols)
        y = np.linspace(0, rows - 1, rows)
        xx, yy = np.meshgrid(x, y)
        grid = xx + 2 * yy

        egrid = extrapolate_grid_edges(grid)

        x_ = np.linspace(-1, cols, cols + 2)
        y_ = np.linspace(-1, rows, rows + 2)
        xx_, yy_ = np.meshgrid(x_, y_)
        grid_ = xx_ + 2 * yy_

        self.assertEqual(egrid.shape, grid_.shape)
        self.assertAlmostEqual(np.min(egrid), np.min(grid_), 7)
        self.assertAlmostEqual(np.max(egrid), np.max(grid_), 7)
        self.assertAlmostEqual(egrid[0, 0], grid_[0, 0], 7)
        self.assertAlmostEqual(egrid[0, -1], grid_[0, -1], 7)
        self.assertAlmostEqual(egrid[-1, 0], grid_[-1, 0], 7)
        self.assertAlmostEqual(egrid[-1, -1], grid_[-1, -1], 7)
        self.assertAlmostEqual(egrid[1, 1], grid_[1, 1], 7)
        self.assertAlmostEqual(egrid[1, 2], grid_[1, 2], 7)
        self.assertAlmostEqual(egrid[2, 2], grid_[2, 2], 7)

    def test_fractional_map(self):

        rows, cols = 11, 11
        N = rows * cols
        xv = np.linspace(0, 10, cols)
        yv = np.linspace(0, 10, rows)
        xx, yy = np.meshgrid(xv, yv)

        # linear case to ensure exact answers
        zgrid = xx + 2 * yy
        self.assertEqual(zgrid.shape, (rows, cols))
        self.assertAlmostEqual(np.min(zgrid), 0, 7)
        self.assertAlmostEqual(np.max(zgrid), 30, 7)

        data = [
            # bin_edges and pixel area below contour
            ([-10, 15, 40], [0, 60.5, 121]),
            ([-10, 0, 10, 20, 30, 40], [0, 0.5625, 33, 88, 120.4375, 121]),
        ]

        for bin_edges, contour_area in data:
            fmap, fwgts = fractional_map([zgrid], [0], bin_edges)
            self.assertEqual(len(fwgts), len(bin_edges) - 1)
            self.assertEqual(len(fmap), len(fwgts))
            sum_wgts = np.sum([np.sum(v) for _, v in fwgts.items()])
            self.assertAlmostEqual(sum_wgts, N, 7)
            weights = np.diff(contour_area)
            for i, wgt in enumerate(weights):
                self.assertAlmostEqual(sum(fwgts[i]), wgt)


if __name__ == "__main__":
    unittest.main()
