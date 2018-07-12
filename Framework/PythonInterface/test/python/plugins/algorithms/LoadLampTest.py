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
        self.assertEquals(ws.readY(0)[350],1134)
        self.assertAlmostEqual(ws.readX(0)[0],-11.8563,4)
        self.assertAlmostEqual(ws.readE(0)[1300],54.2494,4)

    def test_2D_ragged(self):
        ws = LoadLamp('967076_LAMP.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(),3199)
        self.assertEquals(ws.getNumberHistograms(),571)
        self.assertEquals(ws.readY(103)[373],73)
        self.assertAlmostEqual(ws.readE(105)[205],80.7279,4)
        self.assertAlmostEqual(ws.readX(7)[8],-27.0516,4)
        self.assertAlmostEqual(ws.getAxis(1).extractValues()[5],-30.8006,4)

    def test_2D_tile(self):
        ws = LoadLamp('199902_LAMP.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(),2048)
        self.assertEquals(ws.getNumberHistograms(),18)
        self.assertAlmostEqual(ws.readY(8)[1534],0.2546,4)
        self.assertEquals(ws.readX(3)[4],5)
        self.assertAlmostEqual(ws.readE(10)[1535],0.0012,4)
        self.assertAlmostEqual(ws.getAxis(1).extractValues()[11],95.3,1)

    def test_3D_fail(self):
        self.assertRaises(RuntimeError, LoadLamp, Filename='530333_LAMP.hdf', OutputWorkspace='ws')

    def test_no_param(self):
        ws = LoadLamp('no_parameters.hdf')
        self.assertTrue(ws)
        self.assertEquals(ws.blocksize(), 310)

if __name__ == '__main__':
    unittest.main()
