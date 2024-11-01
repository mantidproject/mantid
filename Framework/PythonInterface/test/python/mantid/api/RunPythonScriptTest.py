# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import mtd, IMDWorkspace, WorkspaceGroup
from mantid.simpleapi import ConvertToEventWorkspace, CreateMDWorkspace, CreateWorkspace, DeleteWorkspace, GroupWorkspaces, RunPythonScript


class RunPythonScriptTest(unittest.TestCase):
    """
    Try out RunPythonScript
    """

    def setUp(self):
        CreateWorkspace(OutputWorkspace="ws", DataX="1,2,3,4", DataY="1,2,3", DataE="1,2,3")

    def tearDown(self):
        mtd.clear()

    # ======================== Success cases =====================================================

    def test_Code_Without_Workspaces_Is_Successful_If_Code_Does_Not_Use_Workspace_References(self):
        code = "x = 5 + 2"
        RunPythonScript(Code=code)

    def test_simplePlus(self):
        code = "Plus(LHSWorkspace=input, RHSWorkspace=input, OutputWorkspace=output)"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace="ws_out")
        ws_out = mtd["ws_out"]

        self.assertAlmostEqual(ws_out.dataY(0)[0], 2.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1], 4.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2], 6.0, 3)

    # Use an operation that sets 'output' to a workspace proxy
    def test_usingOperators(self):
        code = "output = input * 5.0"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace="ws_out")
        ws_out = mtd["ws_out"]

        self.assertAlmostEqual(ws_out.dataY(0)[0], 5.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[1], 10.0, 3)
        self.assertAlmostEqual(ws_out.dataY(0)[2], 15.0, 3)

    def test_input_MatrixWorkspace_has_correct_python_type_when_executed(self):
        code = """from mantid.api import MatrixWorkspace
if not isinstance(input, MatrixWorkspace): raise RuntimeError("Input workspace is not a MatrixWorkspace in Python: Type=%s" % str(type(input)))
"""
        RunPythonScript(InputWorkspace="ws", Code=code)

    def test_input_EventWorkspace_has_correct_python_type_when_executed(self):
        test_eventws = ConvertToEventWorkspace(InputWorkspace="ws")
        code = """from mantid.api import IEventWorkspace
if not isinstance(input, IEventWorkspace): raise RuntimeError("Input workspace is not an IEventWorkspace in Python: Type=%s" % str(type(input)))
"""
        RunPythonScript(InputWorkspace=test_eventws, Code=code)

    # Properties handle MDWorkspace types.
    def test_withMDWorkspace(self):
        CreateMDWorkspace(OutputWorkspace="ws", Dimensions="1", Extents="-10,10", Names="x", Units="m")
        code = "output = input"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace="ws_out")
        ws_out = mtd["ws_out"]
        self.assertTrue(isinstance(ws_out, IMDWorkspace))

        # Check type
        code = """from mantid.api import IMDEventWorkspace
if not isinstance(input, IMDEventWorkspace): raise RuntimeError("Input workspace is not an IMDHistoWorkspace in Python: Type=%s" % str(type(input)))
"""
        RunPythonScript(InputWorkspace=mtd["ws"], Code=code)

    def test_withNoInputWorkspace(self):
        c = RunPythonScript(Code="output = CreateSingleValuedWorkspace(DataValue='1')")
        self.assertEqual(c.name(), "c")
        self.assertEqual(c.readY(0)[0], 1)

    def test_algorithm_executes_once_for_whole_input_group_and_not_once_per_group_member(self):
        DeleteWorkspace("ws")
        CreateWorkspace(OutputWorkspace="ws_1", DataX="1,2,3,4", DataY="1,2,3", DataE="1,2,3")
        CreateWorkspace(OutputWorkspace="ws_2", DataX="1,2,3,4", DataY="1,2,3", DataE="1,2,3")
        GroupWorkspaces(InputWorkspaces="ws_1,ws_2", OutputWorkspace="ws")
        self.assertEqual(3, mtd.size())

        code = "Scale(input,OutputWorkspace=output,Factor=5)"
        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace="ws")

        self.assertEqual(3, mtd.size())
        group = mtd["ws"]
        self.assertTrue(isinstance(group, WorkspaceGroup))
        self.assertAlmostEqual(5.0, mtd["ws_1"].readY(0)[0], 8)

    def test_code_with_a_mixture_of_line_endings_succeeds(self):
        code = "Scale(input,OutputWorkspace=output,Factor=5)\n"
        code += "Scale(input,OutputWorkspace=output,Factor=10,Operation='Add')\r"
        code += "Scale(input,OutputWorkspace=output,Factor=20)\r\n"

        RunPythonScript(InputWorkspace="ws", Code=code, OutputWorkspace="ws")
        ws = mtd["ws"]
        self.assertAlmostEqual(300.0, ws.readY(0)[0], 8)

    # ======================== Failure cases =====================================================
    def test_syntax_error_in_code_raises_Runtime_Error(self):
        code = "print 'unclosed quote"
        self.assertRaises(RuntimeError, RunPythonScript, Code=code)

    def test_Code_Without_Workspaces_Fails_If_Code_Uses_Workspace_References(self):
        code = "output = input*5"
        self.assertRaises(RuntimeError, RunPythonScript, Code=code)

    def test_Code_Not_Creating_Output_Workspace_Fails_If_OutputWS_Is_Specified(self):
        code = "x = 5"
        self.assertRaises(RuntimeError, RunPythonScript, Code=code, OutputWorkspace="ws_out")


if __name__ == "__main__":
    unittest.main()
