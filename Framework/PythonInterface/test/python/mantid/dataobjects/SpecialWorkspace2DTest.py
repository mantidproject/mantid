# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name, too-many-public-methods
import unittest

from mantid.simpleapi import Load
from mantid.dataobjects import SpecialWorkspace2D


class SpecialWorkspace2DTest(unittest.TestCase):

    def setUp(self):
        ws = Load("ENGINX228061.nxs")  # SWS2D must have an instrument component, using an ENGINX ws to generate
        self.special_ws2d = SpecialWorkspace2D(ws)

    def test_get_detectorID(self):
        self.assertEqual(100001, self.special_ws2d.getDetectorIDs(0)[0])

    def test_set_value(self):
        self.special_ws2d.setValue(100001, 1)
        val = self.special_ws2d.getValue(100001)
        self.assertEqual(1, val)

    def test_get_value(self):
        self.assertEqual(0, self.special_ws2d.getValue(100001))  # default sws2d values are all 0


if __name__ == '__main__':
    unittest.main()
