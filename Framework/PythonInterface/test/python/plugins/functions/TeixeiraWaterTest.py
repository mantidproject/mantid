from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import *
from mantid.simpleapi import *

class TeixeiraWaterTest(unittest.TestCase):

    def testFunctionIsRegistered(self):
        try:
            FunctionFactory.createFunction("TeixeiraWater")
        except RuntimeError as exc:
            self.fail("Could not create TeixeiraWater function: %s" % str(exc))

    def testEvaluate(self):
        """
        Test function values with parameters Tau=1.5 and L=0.1
        """
        ws = CreateSampleWorkspace('Histogram', 'Flat background', XMin=0, XMax=30, BinWidth=0.1, NumBanks=1)
        EvaluateFunction('name=TeixeiraWater, Tau=1.5, L=0.1', 'ws', StartX=0, EndX=30, OutputWorkspace='out')
        out = mtd['out']
        self.assertTrue(out.readY(1)[0] < 0.0001)
        self.assertTrue(out.readY(1)[149] > 0.46)
        self.assertTrue(out.readY(1)[299] > 0.59)
        DeleteWorkspace(ws)

if __name__ == '__main__':
    unittest.main()
