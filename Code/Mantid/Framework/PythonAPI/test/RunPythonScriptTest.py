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
        pass
        CreateWorkspace(OutputWorkspace='ws',DataX='1,2,3,4',DataY='1,2,3',DataE='1,2,3')
        
    def test_emptyCode_inPlace(self):
        RunPythonScript(InputWorkspace='ws', Code='', OutputWorkspace='ws')
        ws_out = mtd['ws']
        self.assertAlmostEqual(ws_out.dataY(0)[0], 1.0, 3)

        
    def test_emptyCode(self):
        # When not done in-place and the script forgot to do anything,
        # then RunPythonScript() clones the input
        code = ""
        RunPythonScript(InputWorkspace="ws", Code="", OutputWorkspace='ws_out')
        ws = mtd['ws']
        ws_out = mtd['ws_out']
        # We can change the output
        ws_out.dataY(0)[0] = 123.0
        self.assertAlmostEqual(ws_out.dataY(0)[0], 123.0, 3)
        # Without modifying the original, because it was cloned.
        self.assertAlmostEqual(ws.dataY(0)[0], 1.0, 3)
        
        
    def test_simplePlus(self):
        code = "Plus(LHSWorkspace=input, RHSWorkspace=input, OutputWorkspace=output)"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws_out')
        ws_out = mtd['ws_out']
        
        self.assertAlmostEqual(ws_out.dataY(0)[0], 2.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1], 4.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2], 6.0, 3)

        
    # Use an operation that sets 'output' to a workspace proxy
    def test_usingOperators(self):
        code = "output = input * 5.0"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws_out')
        ws_out = mtd['ws_out']
        
        self.assertAlmostEqual(ws_out.dataY(0)[0], 5.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1],10.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2],15.0, 3)

        
    # Properties handle MDWorkspace types.
    def test_withMDWorkspace(self):
        CreateMDWorkspace(OutputWorkspace="ws", Dimensions='1', Extents='-10,10', Names='x', Units='m')
        code = ""
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws_out')
        ws_out = mtd['ws_out']

if __name__ == '__main__':
    unittest.main()

