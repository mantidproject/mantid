# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantid.api import IEventWorkspace, MatrixWorkspace, WorkspaceGroup
from mantid.simpleapi import CreateSampleWorkspace, ReflectometryISISPreprocess


class ReflectometryISISPreprocessTest(unittest.TestCase):
    def test_input_run_is_loaded_histo_mode_by_default(self):
        args = {'InputRunList': '13460',
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_histo_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": False,
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, MatrixWorkspace)
        self.assertEqual("Workspace2D", output_ws.id())

    def test_input_run_is_loaded_event_mode(self):
        args = {'InputRunList': '13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}
        output_ws = ReflectometryISISPreprocess(**args)
        self.assertIsInstance(output_ws, IEventWorkspace)

    @mock.patch("ReflectometryISISPreprocess.LoadEventNexus")
    def test_validation_of_event_workspaces_without_proton_charge_throws(self, mocked_loader):
        args = {'InputRunList': '13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}

        ws = CreateSampleWorkspace()
        mocked_loader.return_value.OutputWorkspace = ws
        with self.assertRaisesRegex(RuntimeError, "proton_charge"):
            ReflectometryISISPreprocess(**args)

    @mock.patch("ReflectometryISISPreprocess.LoadEventNexus")
    def test_validation_of_event_workspace_group_throws(self, mocked_loader):
        args = {'InputRunList': '13460',
                "EventMode": True,
                "OutputWorkspace": "ws"}

        ws = WorkspaceGroup()
        mocked_loader.return_value.OutputWorkspace = ws
        with self.assertRaisesRegex(RuntimeError, "Workspace Groups"):
            ReflectometryISISPreprocess(**args)


if __name__ == '__main__':
    unittest.main()
