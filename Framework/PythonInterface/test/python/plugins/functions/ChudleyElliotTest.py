from __future__ import (absolute_import, division, print_function)

import unittest
from mantid.api import *
from mantid.simpleapi import *

class ChudleyElliotTest(unittest.TestCase):

    def testFunctionIsRegistered(self):
        try:
            FunctionFactory.createFunction("ChudleyElliot")
        except RuntimeError as exc:
            self.fail("Could not create ChudleyElliot function: %s" % str(exc))

    def testEvaluate(self):
        """
        Test function values with parameters Tau=5 and L=1.5
        """
        ws = CreateSampleWorkspace('Histogram', 'Flat background', XMin=0, XMax=30, BinWidth=0.1, NumBanks=1)
        EvaluateFunction('name=ChudleyElliot, Tau=5, L=1.5', 'ws', StartX=0, EndX=30, OutputWorkspace='out')
        out = mtd['out']
        self.assertTrue(out.readY(1)[0] - 0.00018 > 0)
        self.assertTrue(out.readY(1)[30] - 0.24 > 0)
        self.assertTrue(out.readY(1)[299] - 0.19639 > 0)
        DeleteWorkspace(ws)

if __name__ == '__main__':
    unittest.main()
