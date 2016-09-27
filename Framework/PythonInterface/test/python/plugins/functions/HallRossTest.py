from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import *
from mantid.simpleapi import *

class HallRossTest(unittest.TestCase):

    def testFunctionIsRegistered(self):
        try:
            FunctionFactory.createFunction("HallRoss")
        except RuntimeError as exc:
            self.fail("Could not create HallRoss function: %s" % str(exc))

    def testEvaluate(self):
        """
        Test function values with parameters Tau=1.5 and L=0.2
        """
        ws = CreateSampleWorkspace('Histogram', 'Flat background', XMin=0, XMax=30, BinWidth=0.1, NumBanks=1)
        EvaluateFunction('name=HallRoss, Tau=1.5, L=0.2', 'ws', StartX=0, EndX=30, OutputWorkspace='out')
        out = mtd['out']
        self.assertTrue(out.readY(1)[0] < 0.0001)
        self.assertTrue(out.readY(1)[149] > 0.65)
        self.assertTrue(out.readY(1)[299] > 0.66)
        DeleteWorkspace(ws)

if __name__ == '__main__':
    unittest.main()
