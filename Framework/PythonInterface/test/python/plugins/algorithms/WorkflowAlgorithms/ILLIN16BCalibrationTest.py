from __future__ import (absolute_import, division, print_function)

import unittest
import mantid
from mantid.simpleapi import ILLIN16BCalibration
from testhelpers.tempfile_wrapper import TemporaryFileHelper


SAMPLE_GROUPING_XML = \
'''<?xml version="1.0" encoding="utf-8"?>
<detector-grouping instrument="IN16B">
  <group name="psd1"> <ids val="2-129"/> </group>
  <group name="psd2"> <ids val="130-257"/> </group>
  <group name="psd3"> <ids val="258-385"/> </group>
  <group name="psd4"> <ids val="386-513"/> </group>
</detector-grouping>
'''


class ILLIN16BCalibrationTest(unittest.TestCase):

    def test_happy_case_normal(self):
        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=False,
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 18)
        self.assertEqual(calib_ws.blocksize(), 1)


    def test_happy_case_mirror_mode(self):
        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=True,
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 18)
        self.assertEqual(calib_ws.blocksize(), 1)


    def test_map_file(self):
        temp_map = TemporaryFileHelper(SAMPLE_GROUPING_XML, extension='.xml')

        calib_ws = ILLIN16BCalibration(Run='ILLIN16B_034745.nxs',
                                       MirrorMode=True,
                                       MapFile=temp_map.getName(),
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 4)
        self.assertEqual(calib_ws.blocksize(), 1)


if __name__=="__main__":
    unittest.main()
