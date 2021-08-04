# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.api import IEventWorkspace, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace
from plugins.algorithms.WorkflowAlgorithms.ReflectometryISISPreprocess import ReflectometryISISPreprocess
from testhelpers import create_algorithm


class ReflectometryISISPreprocessTest(unittest.TestCase):
    def test_input_run_is_loaded_histo_mode_by_default(self):
        args = {'InputRunList': '13460',
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_histo_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": False,
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_event_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}
        output_ws = self._run_test(args)
        self.assertIsInstance(output_ws, IEventWorkspace)

    def test_validation_of_event_workspaces_without_proton_charge_throws(self):
        ws = CreateSampleWorkspace()

        with self.assertRaisesRegex(RuntimeError, "proton_charge"):
            ReflectometryISISPreprocess._validate_event_ws(ws)

    def test_validation_of_event_workspace_group_throws(self):
        ws = WorkspaceGroup()

        with self.assertRaisesRegex(RuntimeError, "Workspace Groups"):
            ReflectometryISISPreprocess._validate_event_ws(ws)

    def _run_test(self, args):
        alg = create_algorithm('ReflectometryISISPreprocess', **args)
        alg.setChild(True)
        alg.setRethrows(True)
        alg.execute()
        output_ws = alg.getProperty('OutputWorkspace').value
        return output_ws


if __name__ == '__main__':
    unittest.main()
