# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
import json
from mantid.kernel import V3D, SpecialCoordinateSystem
from mantid.geometry import PeakShape
from mantid.dataobjects import NoShape, PeakShapeSpherical, PeakShapeEllipsoid


class PeakShapeDataObjects(unittest.TestCase):
    def test_NoShape(self):
        shape = NoShape()

        self.assertTrue(isinstance(shape, NoShape))
        self.assertTrue(isinstance(shape, PeakShape))

        self.assertEqual(shape.shapeName(), "none")
        self.assertEqual(shape.toJSON(), '{"shape":"none"}')

    def test_PeakShapeSpherical(self):
        sphere1 = PeakShapeSpherical(0.5)

        self.assertTrue(isinstance(sphere1, PeakShapeSpherical))
        self.assertTrue(isinstance(sphere1, PeakShape))

        self.assertEqual(sphere1.shapeName(), "spherical")

        data = json.loads(sphere1.toJSON())
        self.assertEqual(data["radius"], 0.5)
        self.assertEqual(data["shape"], "spherical")
        self.assertEqual(data["frame"], 2)
        self.assertTrue("background_inner_radius" not in data)
        self.assertTrue("background_outer_radius" not in data)

        sphere2 = PeakShapeSpherical(0.5, 0.6, 0.7, SpecialCoordinateSystem.HKL)
        data = json.loads(sphere2.toJSON())
        self.assertEqual(data["radius"], 0.5)
        self.assertEqual(data["shape"], "spherical")
        self.assertEqual(data["frame"], 3)
        self.assertEqual(data["background_inner_radius"], 0.6)
        self.assertEqual(data["background_outer_radius"], 0.7)

    def test_PeakShapeEllipsoid(self):
        ellipse = PeakShapeEllipsoid([V3D(1, 0, 0), V3D(0, 1, 0), V3D(0, 0, 1)], [0.1, 0.2, 0.3], [0.4, 0.5, 0.6], [0.7, 0.8, 0.9])

        self.assertTrue(isinstance(ellipse, PeakShapeEllipsoid))
        self.assertTrue(isinstance(ellipse, PeakShape))
        self.assertEqual(ellipse.shapeName(), "ellipsoid")

        data = json.loads(ellipse.toJSON())
        self.assertEqual(data["shape"], "ellipsoid")
        self.assertEqual(data["frame"], 2)
        self.assertEqual(data["direction0"], "1 0 0")
        self.assertEqual(data["direction1"], "0 1 0")
        self.assertEqual(data["direction2"], "0 0 1")
        self.assertEqual(data["radius0"], 0.1)
        self.assertEqual(data["radius1"], 0.2)
        self.assertEqual(data["radius2"], 0.3)
        self.assertEqual(data["background_inner_radius0"], 0.4)
        self.assertEqual(data["background_inner_radius1"], 0.5)
        self.assertEqual(data["background_inner_radius2"], 0.6)
        self.assertEqual(data["background_outer_radius0"], 0.7)
        self.assertEqual(data["background_outer_radius1"], 0.8)
        self.assertEqual(data["background_outer_radius2"], 0.9)


if __name__ == "__main__":
    unittest.main()
