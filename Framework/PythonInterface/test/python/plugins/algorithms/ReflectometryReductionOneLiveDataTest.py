from __future__ import (absolute_import, division, print_function)
import unittest
from testhelpers import run_algorithm
from mantid.simpleapi import mtd


class ReflectometryReducctionOneLiveDataTest(unittest.TestCase):

    _ws = None
    
    def setUp(self):
        dataX = [0, 1]
        dataY = [0]
        dataE = [0]
        nSpec = 1
        no_peak_ws_alg = run_algorithm("CreateWorkspace", DataX=dataX, DataY=dataY, DataE=dataE, NSpec=nSpec, UnitX="Wavelength", VerticalAxisUnit="SpectraNumber", OutputWorkspace="no_peak_ws")
        self.__class__._ws = no_peak_ws_alg.getPropertyValue("OutputWorkspace")

    def tearDown(self):
        mtd.clear()

    def test

if __name__ == "__main__":
    unittest.main()
