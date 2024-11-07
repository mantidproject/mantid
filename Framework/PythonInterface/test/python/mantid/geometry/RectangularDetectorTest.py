# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from mantid.geometry import RectangularDetector
from testhelpers import can_be_instantiated, WorkspaceCreationHelper


class RectangularDetectorTest(unittest.TestCase):
    def test_RectangularDetector_cannot_be_instantiated(self):
        self.assertFalse(can_be_instantiated(RectangularDetector))

    def test_RectangularDetector_has_expected_attributes(self):
        attrs = dir(RectangularDetector)
        expected_attrs = [
            "idfillbyfirst_y",
            "idstart",
            "idstep",
            "idstepbyrow",
            "maxDetectorID",
            "minDetectorID",
            "xpixels",
            "xsize",
            "xstart",
            "xstep",
            "ypixels",
            "ysize",
            "ystart",
            "ystep",
            "type",
            "nelements",
        ]
        for att in expected_attrs:
            self.assertTrue(att in attrs)

    def test_RectangularDetector_getattributes(self):
        testws = WorkspaceCreationHelper.create2DWorkspaceWithRectangularInstrument(3, 5, 5)
        i = testws.getInstrument()
        self.assertEqual(i[2].getName(), "bank3")
        self.assertEqual(i[2][2].getName(), "bank3(x=2)")
        self.assertEqual(i[2][2][2].getName(), "bank3(2,2)")
        self.assertEqual(i[2].nelements(), 5)
        self.assertEqual(i[2].xstart() + i[2].xstep() * i[2].xpixels(), 0.04)
        self.assertEqual(i[1].ystart() + i[1].ystep() * i[1].ypixels(), 0.04)
        self.assertEqual(i[0].xsize(), 0.04)
        self.assertEqual(i[2].idstart(), 75)
        self.assertEqual(i[0].idstep(), 1)
        self.assertEqual(i[1].idstepbyrow(), 5)
        self.assertEqual(i[1].maxDetectorID(), 74)
        self.assertEqual(i[1].minDetectorID(), 50)


if __name__ == "__main__":
    unittest.main()
