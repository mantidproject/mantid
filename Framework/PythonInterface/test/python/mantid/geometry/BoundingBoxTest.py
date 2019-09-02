# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.geometry import BoundingBox
from mantid.kernel import V3D

class BoundingBoxTest(unittest.TestCase):

    def test_default_construction_is_allowed(self):
        box = BoundingBox()
        self.assertTrue(isinstance(box, BoundingBox))
        self.assertTrue(box.isNull())

    def test_construction_with_min_max_values_is_allowed(self):
        box = BoundingBox(1.0, 4.0, 5.0, 0.0, 2.0, 3.0)
        self.assertTrue(isinstance(box, BoundingBox))

    def test_properties_are_correct(self):
        bbox = BoundingBox (1.0, 2.0, 3.0, -1.0, -2.0, -3.0)
        self.assertEqual(bbox.minPoint(), V3D(-1.0,-2.0,-3.0))
        self.assertEqual(bbox.maxPoint(), V3D(1.0,2.0,3.0))
        self.assertEqual(bbox.centrePoint(), V3D(0.0,0.0,0.0))
        self.assertEqual(bbox.width(), V3D(2.0,4.0,6.0))

    def test_point_inside(self):
        box = BoundingBox(1.0, 2.0, 3.0, -1.0, -2.0, -3.0)
        self.assertTrue(box.isPointInside(V3D(0.0,0.0,0.0)))

    def test_doesLineIntersect(self):
        bbox  = BoundingBox(4.1, 4.1, 4.1, -4.1, -4.1, -4.1)
        self.assertTrue(bbox.doesLineIntersect(V3D(-6.0,0.0,0.0), V3D(1.0,0.0,0.0)))

if __name__ == '__main__':
    unittest.main()
