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
        component_info = testws.componentInfo()
        bank1 = component_info.indexOfAny("bank1")
        bank2 = component_info.indexOfAny("bank2")
        bank3 = component_info.indexOfAny("bank3")
        self.assertEqual(component_info.name(bank3), "bank3")
        column = int(component_info.children(bank3)[2])
        self.assertEqual(component_info.name(column), "bank3(x=2)")
        self.assertEqual(component_info.name(int(component_info.children(column)[2])), "bank3(2,2)")
        self.assertEqual(len(component_info.children(bank3)), 5)
        self.assertEqual(
            component_info.pixelGridXStart(bank3) + component_info.pixelGridXStep(bank3) * component_info.pixelGridNX(bank3), 0.04
        )
        self.assertEqual(
            component_info.pixelGridYStart(bank2) + component_info.pixelGridYStep(bank2) * component_info.pixelGridNY(bank2), 0.04
        )
        # the legacy xsize() is xpixels * xstep
        self.assertEqual(component_info.pixelGridNX(bank1) * component_info.pixelGridXStep(bank1), 0.04)
        self.assertEqual(component_info.pixelGridIdStart(bank3), 75)
        self.assertEqual(component_info.pixelGridIdStep(bank1), 1)
        self.assertEqual(component_info.pixelGridIdStepByRow(bank2), 5)
        self.assertEqual(component_info.pixelGridMaxDetectorID(bank2), 74)
        self.assertEqual(component_info.pixelGridMinDetectorID(bank2), 50)


if __name__ == "__main__":
    unittest.main()
