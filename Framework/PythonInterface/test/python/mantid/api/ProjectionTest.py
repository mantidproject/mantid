# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.api import Projection
from mantid.kernel import V3D
from mantid.simpleapi import mtd
import numpy as np


class ProjectionTest(unittest.TestCase):
    def test_constructors(self):
        p = Projection(V3D(0, 1, 0), V3D(-1, 1, 0), V3D(0, 0, 1))
        self.assertEqual(p.getAxis(0), V3D(0, 1, 0))
        self.assertEqual(p.getAxis(1), V3D(-1, 1, 0))
        self.assertEqual(p.getAxis(2), V3D(0, 0, 1))

    def test_accessors(self):
        p = Projection()

        p.setAxis(0, V3D(0, 1, 2))
        p.setAxis(1, V3D(3, 4, 5))
        p.setAxis(2, V3D(6, 7, 8))
        self.assertEqual(p.getAxis(0), V3D(0, 1, 2))
        self.assertEqual(p.getAxis(1), V3D(3, 4, 5))
        self.assertEqual(p.getAxis(2), V3D(6, 7, 8))

        p.setOffset(0, 1)
        p.setOffset(1, 4)
        p.setOffset(2, 9)
        self.assertEqual(p.getOffset(0), 1)
        self.assertEqual(p.getOffset(1), 4)
        self.assertEqual(p.getOffset(2), 9)

        p.setType(0, "r")
        p.setType(1, "a")
        p.setType(2, "r")
        self.assertEqual(p.getType(0), "r")
        self.assertEqual(p.getType(1), "a")
        self.assertEqual(p.getType(2), "r")

    def test_setaxis_with_numpy_float(self):
        p = Projection()

        p.setAxis(0, np.array([0, 1, 2], dtype="float64"))
        p.setAxis(1, np.array([3, 4, 5], dtype="float64"))
        p.setAxis(2, np.array([6, 7, 8], dtype="float64"))
        self.assertEqual(p.getAxis(0), V3D(0, 1, 2))
        self.assertEqual(p.getAxis(1), V3D(3, 4, 5))
        self.assertEqual(p.getAxis(2), V3D(6, 7, 8))

    def test_setaxis_with_numpy_int64(self):
        p = Projection()

        p.setAxis(0, np.array([1, 2, 3], dtype="int64"))
        p.setAxis(1, np.array([3, 4, 5], dtype="int64"))
        p.setAxis(2, np.array([6, 7, 8], dtype="int64"))
        self.assertEqual(p.getAxis(0), V3D(1, 2, 3))
        self.assertEqual(p.getAxis(1), V3D(3, 4, 5))
        self.assertEqual(p.getAxis(2), V3D(6, 7, 8))

    def test_uvw(self):
        p = Projection()
        p.setAxis(0, V3D(0, 1, 2))
        p.setAxis(1, V3D(3, 4, 5))
        p.setAxis(2, V3D(6, 7, 8))

        self.assertEqual(p.u, V3D(0, 1, 2))
        self.assertEqual(p.v, V3D(3, 4, 5))
        self.assertEqual(p.w, V3D(6, 7, 8))

        p.u = V3D(2, 3, 0)
        p.v = V3D(7, 8, 9)
        p.w = V3D(4, 6, 0)

        self.assertEqual(p.u, V3D(2, 3, 0))
        self.assertEqual(p.v, V3D(7, 8, 9))
        self.assertEqual(p.w, V3D(4, 6, 0))

    def test_ads(self):
        p = Projection()
        p.setAxis(0, V3D(0, 1, 2))
        p.setAxis(1, V3D(3, -4, 5))
        p.setAxis(2, V3D(6, 7, 8.5))
        p.setOffset(1, 0.15)
        p.setType(2, "a")
        proj_test_ads = p.createWorkspace()
        p.createWorkspace(OutputWorkspace="proj_test_ads2")
        self.assertTrue("proj_test_ads" in mtd, msg="Workspace not added to ADS successfully")
        self.assertTrue("proj_test_ads2" in mtd, msg="Workspace not added to ADS successfully")

        self.assertEqual(proj_test_ads.row(0), {"name": "u", "value": V3D(0, 1, 2), "type": "r", "offset": 0.0})
        self.assertEqual(proj_test_ads.row(1), {"name": "v", "value": V3D(3, -4, 5), "type": "r", "offset": 0.15})
        self.assertEqual(proj_test_ads.row(2), {"name": "w", "value": V3D(6, 7, 8.5), "type": "a", "offset": 0.0})


if __name__ == "__main__":
    unittest.main()
