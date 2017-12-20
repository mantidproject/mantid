from __future__ import (absolute_import, division, print_function)
from mantid.simpleapi import config, mtd, LoadLamp
import unittest


class LoadLampTest(unittest.TestCase):

    def setUp(self):
        config.appendDataSearchSubDir('ILL/LAMP/')

    def tearDown(self):
        mtd.clear()

    def test_1D(self):
        ws = LoadLamp('967067_LAMP.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(),3200)
        self.assertEquals(ws.getNumberHistograms(),1)

    def test_2D_ragged(self):
        ws = LoadLamp('967076_LAMP.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(),3199)
        self.assertEquals(ws.getNumberHistograms(),571)

    def test_2D_tile(self):
        ws = LoadLamp('199902_LAMP.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(),2048)
        self.assertEquals(ws.getNumberHistograms(),18)

    def test_3D_fail(self):
        self.assertRaises(RuntimeError, LoadLamp, Filename='530333_LAMP.hdf', OutputWorkspace='ws')

if __name__ == '__main__':
    unittest.main()
