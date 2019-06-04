# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import math
import numpy as np
from mantid.kernel import V3D


class V3DTest(unittest.TestCase):
    def test_default_construction_is_at_origin(self):
        p = V3D()
        self.assertEqual(p.X(), 0.0)
        self.assertEqual(p.Y(), 0.0)
        self.assertEqual(p.Z(), 0.0)

    def test_construction_with_xyz(self):
        p = V3D(1.5, 2.4, 6.5)
        self.assertEqual(p.X(), 1.5)
        self.assertEqual(p.Y(), 2.4)
        self.assertEqual(p.Z(), 6.5)

    def test_distance(self):
        a = V3D(0.0, 0.0, 0.0)
        b = V3D(2.0, 2.0, 2.0)
        d = a.distance(b)
        self.assertAlmostEquals(d, 2.0 * math.sqrt(3.0))

    def test_angle(self):
        a = V3D(2.0, 0.0, 0.0)
        b = V3D(0.0, 1.0, 0.0)
        c = V3D(1.0, 1.0, 0.0)
        d = V3D(-1.0, 0.0, 0.0)
        self.assertAlmostEquals(a.angle(a), 0.0)
        self.assertAlmostEquals(a.angle(b), math.pi / 2.0)
        self.assertAlmostEquals(a.angle(c), math.pi / 4.0)
        self.assertAlmostEquals(a.angle(d), math.pi)

    def test_zenith(self):
        b = V3D(0.0, 0.0, 0.0)
        a = V3D(9.9, 7.6, 0.0)
        self.assertEqual(a.zenith(a), 0.0)
        self.assertAlmostEquals(a.zenith(b), math.pi / 2.0)
        a = V3D(-1.1, 0.0, 0.0);
        self.assertAlmostEquals(a.zenith(b), math.pi / 2.0)
        a = V3D(0.0, 0.0, 1.0);
        self.assertEqual(a.zenith(b), 0.0);
        a = V3D(1.0, 0.0, 1.0);
        self.assertAlmostEquals(a.zenith(b), math.pi / 4.0)
        a = V3D(1.0, 0.0, -1.0);
        self.assertAlmostEquals(a.zenith(b), 3.0 * math.pi / 4.0)

    def test_scalarprod(self):
        a = V3D(1.0, 2.0, 1.0)
        b = V3D(1.0, -2.0, -1.0)
        sp = a.scalar_prod(b)
        self.assertAlmostEquals(sp, -4.0)

    def test_crossprod(self):
        a = V3D(1.0, 0.0, 0.0)
        b = V3D(0.0, 1.0, 0.0)
        c = a.cross_prod(b)
        self.assertAlmostEquals(c.X(), 0.0)
        self.assertAlmostEquals(c.Y(), 0.0)
        self.assertAlmostEquals(c.Z(), 1.0)

    def test_norm(self):
        p = V3D(1.0, -5.0, 8.0);
        self.assertAlmostEquals(p.norm(), math.sqrt(90.0))

    def test_norm2(self):
        p = V3D(1.0, -5.0, 8.0);
        self.assertAlmostEquals(p.norm2(), 90.0)

    def test_equality_operators_use_value_comparison(self):
        p1 = V3D(1.0, -5.0, 8.0)
        p2 = V3D(1.0, -5.0, 8.0)
        self.assertEqual(p1,  p2)

    def test_inequality_operators_use_value_comparison(self):
        p1 = V3D(1.0, -5.0, 8.0)
        p2 = V3D(1.0, -5.0, 8.0)  # different objects, same value
        self.assertFalse(p1 != p2)
        p3 = V3D(1.0, -5.0, 10.0)
        self.assertNotEqual(p1,  p3)

    def test_directionAngles_rads(self):
        v = V3D(1, 1, 1)
        inDegrees = False
        angles = v.directionAngles(inDegrees)
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)), angles.X())
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)), angles.Y())
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)), angles.Z())

    def test_directionAngles(self):
        v = V3D(1, 1, 1)
        angles = v.directionAngles()
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)) * 180 / math.pi, angles.X())
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)) * 180 / math.pi, angles.Y())
        self.assertAlmostEquals(math.acos(1.0 / math.sqrt(3.0)) * 180 / math.pi, angles.Z())

    def test_hash(self):
        v1 = V3D(1, 1, 1)
        v2 = V3D(1, 1, 1)
        v3 = V3D(1, 0, 0)

        a = set([v1, v2, v3])

        self.assertEqual(len(a), 2)

    def test_get_item(self):
        v = V3D(2, 1, 3)

        self.assertRaises(IndexError, v.__getitem__, 3)
        self.assertRaises(IndexError, v.__getitem__, -4)

        self.assertEqual(v[0], 2.0)
        self.assertEqual(v[1], 1.0)
        self.assertEqual(v[2], 3.0)

        self.assertEqual(v[-3], 2.0)
        self.assertEqual(v[-2], 1.0)
        self.assertEqual(v[-1], 3.0)

    def test_set_item(self):
        v = V3D(2, 1, 3)

        self.assertRaises(IndexError, v.__setitem__, 3, 0.0)
        self.assertRaises(IndexError, v.__setitem__, -4, 0.0)

        v[0] = 1.0
        v[1] = 2.0
        v[2] = 4.0

        self.assertEqual(v[0], 1.0)
        self.assertEqual(v[1], 2.0)
        self.assertEqual(v[2], 4.0)

        v[-3] = 3.0
        v[-2] = 5.0
        v[-1] = 6.0

        self.assertEqual(v[0], 3.0)
        self.assertEqual(v[1], 5.0)
        self.assertEqual(v[2], 6.0)

    def test_iterator(self):
        times_two = [2 * x for x in V3D(3, 4, 5)]

        self.assertEqual(times_two[0], 6.0)
        self.assertEqual(times_two[1], 8.0)
        self.assertEqual(times_two[2], 10.0)

    def test_len(self):
        self.assertEqual(len(V3D(2, 2, 2)), 3)

    def test_numpy_conversion(self):
        v = V3D(1, 2, 3)
        v_as_numpy = np.array(v)

        self.assertEqual(np.all(v_as_numpy,  np.array([1, 2, 3])))


if __name__ == '__main__':
    unittest.main()
