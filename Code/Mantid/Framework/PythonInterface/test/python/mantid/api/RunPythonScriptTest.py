import unittest
import os

from mantid.api import *
from mantid.kernel import *
from mantid.simpleapi import *


class RunPythonScriptTest(unittest.TestCase):
    """
    Try out RunPythonScript
    """
    
    def setUp(self):
        CreateWorkspace(OutputWorkspace='ws',DataX='1,2,3,4',DataY='1,2,3',DataE='1,2,3')

    def tearDown(self):
        mtd.clear()

    # ======================== Success cases =====================================================
    
    def test_Code_Without_Workspaces_Is_Successful_If_Code_Does_Not_Use_Workspace_References(self):
        code = "x = 5 + 2"
        RunPythonScript(Code=code)
        
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
        code = "output = input"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace='ws_out')
        ws_out = mtd['ws_out']
        self.assertTrue(isinstance(ws_out, IMDWorkspace))
        
    def test_withNoInputWorkspace(self):
        c = RunPythonScript(Code="output = CreateSingleValuedWorkspace(DataValue='1')")
        self.assertEqual(c.readY(0)[0], 1)

    def test_algorithm_executes_once_for_whole_input_group_and_not_once_per_group_member(self):
        DeleteWorkspace('ws')
        CreateWorkspace(OutputWorkspace='ws_1',DataX='1,2,3,4',DataY='1,2,3',DataE='1,2,3')
        CreateWorkspace(OutputWorkspace='ws_2',DataX='1,2,3,4',DataY='1,2,3',DataE='1,2,3')
        GroupWorkspaces(InputWorkspaces='ws_1,ws_2',OutputWorkspace='ws')
        self.assertEquals(3,mtd.size())

        code = "Scale(input,OutputWorkspace=output,Factor=5)"
        RunPythonScript(InputWorkspace='ws',Code=code,OutputWorkspace='ws')

        self.assertEquals(3,mtd.size())
        group = mtd['ws']
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertAlmostEqual(5.0, mtd['ws_1'].readY(0)[0], 8)

    def xtest_code_with_a_mixture_of_line_endings_succeeds(self):
        code = "Scale(input,OutputWorkspace=output,Factor=5)\n"
        code += "Scale(input,OutputWorkspace=output,Factor=10,Operation='Add')\r"
        code += "Scale(input,OutputWorkspace=output,Factor=20)\r\n"

        RunPythonScript(InputWorkspace='ws',Code=code,OutputWorkspace='ws')
        ws = mtd['ws']
        self.assertAlmostEqual(300.0, mtd['ws'].readY(0)[0], 8)

    # ======================== Failure cases =====================================================
    def test_Code_Without_Workspaces_Fails_If_Code_Uses_Workspace_References(self):
        code = "output = input*5"
        self.assertRaises(TypeError,RunPythonScript,Code=code)

    def test_Code_Not_Creating_Output_Workspace_Fails_If_OutputWS_Is_Specified(self):
        code = "x = 5"
        self.assertRaises(RuntimeError, RunPythonScript, Code=code, OutputWorkspace='ws_out')

if __name__ == '__main__':
    unittest.main()

