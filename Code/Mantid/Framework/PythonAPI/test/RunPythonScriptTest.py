import unittest
import os

from MantidFramework import mtd
mtd.initialise()
from mantidsimple import *


class RunPythonScriptTest(unittest.TestCase):
    """
    Try out RunPythonScript
    """
    
    def setUp(self):
        CreateWorkspace(OutputWorkspace='ws',DataX='1,2,3,4',DataY='1,2,3',DataE='1,2,3')
        
    def test_emptyCode(self):
        code = ""
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws')
        ws_out = mtd['ws']
        
        # Nothing was done to ws_out, which now points to ws
        self.assertAlmostEqual(ws_out.dataY(0)[0], 1.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1], 2.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2], 3.0, 3)
        
    def test_simplePlus(self):
        code = """
Plus(LHSWorkspace=input, RHSWorkspace=input, OutputWorkspace=output)
"""
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws_out')
        ws_out = mtd['ws_out']
        
        self.assertAlmostEqual(ws_out.dataY(0)[0], 2.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1], 4.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2], 6.0, 3)
        

if __name__ == '__main__':
    unittest.main()

