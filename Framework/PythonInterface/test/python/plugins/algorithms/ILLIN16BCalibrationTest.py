from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.simpleapi import *
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

    # cache the def instrument and data search dirs
    _def_fac = config['default.facility']
    _def_inst = config['default.instrument']
    _data_dirs = config['datasearch.directories']

    def setUp(self):
        # set instrument and append datasearch directory
        config['default.facility'] = 'ILL'
        config['default.instrument'] = 'IN16B'
        config.appendDataSearchSubDir('ILL/IN16B/')

    def tearDown(self):
        # set cached facility and datasearch directory
        config['default.facility'] = self._def_fac
        config['default.instrument'] = self._def_inst
        config['datasearch.directories'] = self._data_dirs

    def test_happy_case_normal(self):
        calib_ws = ILLIN16BCalibration(Run='146191',
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 18)
        self.assertEqual(calib_ws.blocksize(), 1)

    def test_multiple_files(self):
        calib_ws = ILLIN16BCalibration(Run='146191,146192',
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 18)
        self.assertEqual(calib_ws.blocksize(), 1)

        self.assertNotEqual(calib_ws.readX(0).all(), 0.0)

    def test_map_file(self):
        temp_map = TemporaryFileHelper(SAMPLE_GROUPING_XML, extension='.xml')

        calib_ws = ILLIN16BCalibration(Run='146191',
                                       MapFile=temp_map.getName(),
                                       PeakRange=[-0.001, 0.002])

        self.assertEqual(calib_ws.getNumberHistograms(), 4)
        self.assertEqual(calib_ws.blocksize(), 1)


if __name__=="__main__":
    unittest.main()
