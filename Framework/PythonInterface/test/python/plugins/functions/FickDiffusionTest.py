from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import *
from mantid.simpleapi import *

class FickDiffusionTest(unittest.TestCase):

    def testFunctionIsRegistered(self):
        try:
            FunctionFactory.createFunction("FickDiffusion")
        except RuntimeError as exc:
            self.fail("Could not create FickDiffusion function: %s" % str(exc))

    def testEvaluate(self):
        """
        Test function values with parameters D=1.5
        """
        ws = CreateSampleWorkspace('Histogram', 'Flat background', XMin=0, XMax=30, BinWidth=0.1, NumBanks=1)
        EvaluateFunction('name=FickDiffusion, D=1.5', 'ws', StartX=0, EndX=30, OutputWorkspace='out')
        out = mtd['out']
        self.assertTrue(out.readY(1)[0] - 0.003 > 0)
        self.assertTrue(out.readY(1)[149] - 335 > 0)
        self.assertTrue(out.readY(1)[299] - 1345 > 0)
        DeleteWorkspace(ws)

if __name__ == '__main__':
    unittest.main()
